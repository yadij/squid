/*
 * Copyright (C) 1996-2023 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 05    Socket Functions */

#include "squid.h"

#if USE_POLL
#include "anyp/PortCfg.h"
#include "comm/Connection.h"
#include "comm/Loops.h"
#include "fd.h"
#include "fde.h"
#include "globals.h"
#include "ICP.h"
#include "mgr/Registration.h"
#include "SquidConfig.h"
#include "StatCounters.h"
#include "Store.h"

#include <cerrno>
#if HAVE_POLL_H
#include <poll.h>
#endif

/* Needed for poll() on Linux at least */
#if USE_POLL
#ifndef POLLRDNORM
#define POLLRDNORM POLLIN
#endif
#ifndef POLLWRNORM
#define POLLWRNORM POLLOUT
#endif
#endif

static int MAX_POLL_TIME = 1000;    /* see also Comm::QuickPollRequired() */

#ifndef        howmany
#define howmany(x, y)   (((x)+((y)-1))/(y))
#endif
#ifndef        NBBY
#define        NBBY    8
#endif
#define FD_MASK_BYTES sizeof(fd_mask)
#define FD_MASK_BITS (FD_MASK_BYTES*NBBY)

/* STATIC */
static int fdIsTcpListen(int fd);
static int fdIsUdpListen(int fd);
static int fdIsDns(int fd);
static int comm_check_incoming_poll_handlers(int nfds, int *fds);
static void comm_poll_dns_incoming(void);

/** Automatic tuning for incoming requests:
 *
 * INCOMING sockets are the ICP and HTTP ports.  We need to check these
 * fairly regularly, but how often?  When the load increases, we
 * want to check the incoming sockets more often.  If we have a lot
 * of incoming ICP, then we need to check these sockets more than
 * if we just have HTTP.
 *
 * \copydoc StatCounter::comm_internal::interval
 *
 * \copydoc StatCounter::comm_internal::ioEvents
 *
 * Every time we check incoming sockets, we count how many new messages
 * or connections were processed.  This is used to adjust the
 * incoming_interval for the next iteration.  The new incoming_interval
 * is calculated as the current incoming_interval plus what we would
 * like to see as an average number of events minus the number of
 * events just processed.
 *
 *  incoming_interval = incoming_interval + target_average - number_of_events_processed
 *
 * There are separate incoming_interval counters for TCP-based, UDP-based, and DNS events
 *
 * You can see the current values of the incoming_interval's, as well as
 * a histogram of 'incoming_events' in the cache manager 'comm_incoming' report.
 *
 * Caveats:
 *
 * \copydoc MAX_INCOMING_INTEGER
 *
 * \copydoc INCOMING_FACTOR
 */

#define MAX_INCOMING_INTERVAL (MAX_INCOMING_INTEGER << INCOMING_FACTOR)
#define commCheckUdpIncoming (++statCounter.udp_incoming.ioEvents > (statCounter.udp_incoming.interval >> INCOMING_FACTOR))
#define commCheckDnsIncoming (++statCounter.dns.ioEvents > (statCounter.dns.interval >> INCOMING_FACTOR))
#define commCheckTcpIncoming (++statCounter.tcp.ioEvents > (statCounter.tcp.interval >> INCOMING_FACTOR))

void
Comm::SetSelect(int fd, unsigned int type, PF * handler, void *client_data, time_t timeout)
{
    fde *F = &fd_table[fd];
    assert(fd >= 0);
    assert(F->flags.open || (!handler && !client_data && !timeout));
    debugs(5, 5, "FD " << fd << ", type=" << type <<
           ", handler=" << handler << ", client_data=" << client_data <<
           ", timeout=" << timeout);

    if (type & COMM_SELECT_READ) {
        F->read_handler = handler;
        F->read_data = client_data;
    }

    if (type & COMM_SELECT_WRITE) {
        F->write_handler = handler;
        F->write_data = client_data;
    }

    if (timeout)
        F->timeout = squid_curtime + timeout;
}

static int
fdIsUdpListen(int fd)
{
    if (icpIncomingConn != nullptr && icpIncomingConn->fd == fd)
        return 1;

    if (icpOutgoingConn != nullptr && icpOutgoingConn->fd == fd)
        return 1;

    return 0;
}

static int
fdIsDns(int fd)
{
    if (fd == DnsSocketA)
        return 1;

    if (fd == DnsSocketB)
        return 1;

    return 0;
}

static int
fdIsTcpListen(int fd)
{
    for (AnyP::PortCfgPointer s = HttpPortList; s != nullptr; s = s->next) {
        if (s->listenConn != nullptr && s->listenConn->fd == fd)
            return 1;
    }

    return 0;
}

static int
comm_check_incoming_poll_handlers(int nfds, int *fds)
{
    int i;
    int fd;
    PF *hdl = nullptr;
    int npfds;

    struct pollfd pfds[3 + MAXTCPLISTENPORTS];
    incoming_sockets_accepted = 0;

    for (i = npfds = 0; i < nfds; ++i) {
        int events;
        fd = fds[i];
        events = 0;

        if (fd_table[fd].read_handler)
            events |= POLLRDNORM;

        if (fd_table[fd].write_handler)
            events |= POLLWRNORM;

        if (events) {
            pfds[npfds].fd = fd;
            pfds[npfds].events = events;
            pfds[npfds].revents = 0;
            ++npfds;
        }
    }

    if (!nfds)
        return -1;

    getCurrentTime();
    ++ statCounter.syscalls.selects;

    if (poll(pfds, npfds, 0) < 1)
        return incoming_sockets_accepted;

    for (i = 0; i < npfds; ++i) {
        int revents;

        if (((revents = pfds[i].revents) == 0) || ((fd = pfds[i].fd) == -1))
            continue;

        if (revents & (POLLRDNORM | POLLIN | POLLHUP | POLLERR)) {
            if ((hdl = fd_table[fd].read_handler)) {
                fd_table[fd].read_handler = nullptr;
                hdl(fd, fd_table[fd].read_data);
            } else if (pfds[i].events & POLLRDNORM)
                debugs(5, DBG_IMPORTANT, "comm_poll_incoming: FD " << fd << " NULL read handler");
        }

        if (revents & (POLLWRNORM | POLLOUT | POLLHUP | POLLERR)) {
            if ((hdl = fd_table[fd].write_handler)) {
                fd_table[fd].write_handler = nullptr;
                hdl(fd, fd_table[fd].write_data);
            } else if (pfds[i].events & POLLWRNORM)
                debugs(5, DBG_IMPORTANT, "comm_poll_incoming: FD " << fd << " NULL write_handler");
        }
    }

    return incoming_sockets_accepted;
}

static void
comm_poll_udp_incoming(void)
{
    int nfds = 0;
    int fds[2];
    int nevents;
    statCounter.udp_incoming.ioEvents = 0;

    if (Comm::IsConnOpen(icpIncomingConn)) {
        fds[nfds] = icpIncomingConn->fd;
        ++nfds;
    }

    if (icpIncomingConn != icpOutgoingConn && Comm::IsConnOpen(icpOutgoingConn)) {
        fds[nfds] = icpOutgoingConn->fd;
        ++nfds;
    }

    if (nfds == 0)
        return;

    nevents = comm_check_incoming_poll_handlers(nfds, fds);

    statCounter.udp_incoming.interval += Config.comm_incoming.udp.average - nevents;

    if (statCounter.udp_incoming.interval < Config.comm_incoming.udp.min_poll)
        statCounter.udp_incoming.interval = Config.comm_incoming.udp.min_poll;

    if (statCounter.udp_incoming.interval > MAX_INCOMING_INTERVAL)
        statCounter.udp_incoming.interval = MAX_INCOMING_INTERVAL;

    if (nevents > INCOMING_UDP_MAX)
        nevents = INCOMING_UDP_MAX;

    statCounter.udp_incoming.history.count(nevents);
}

static void
comm_poll_tcp_incoming(void)
{
    int nfds = 0;
    int fds[MAXTCPLISTENPORTS];
    int j;
    int nevents;
    statCounter.tcp.ioEvents = 0;

    // XXX: only poll sockets that won't be deferred. But how do we identify them?

    for (j = 0; j < NHttpSockets; ++j) {
        if (HttpSockets[j] < 0)
            continue;

        fds[nfds] = HttpSockets[j];
        ++nfds;
    }

    nevents = comm_check_incoming_poll_handlers(nfds, fds);
    statCounter.tcp.interval += Config.comm_incoming.tcp.average - nevents;

    if (statCounter.udp_incoming.interval < Config.comm_incoming.tcp.min_poll)
        statCounter.udp_incoming.interval = Config.comm_incoming.tcp.min_poll;

    if (statCounter.udp_incoming.interval > MAX_INCOMING_INTERVAL)
        statCounter.udp_incoming.interval = MAX_INCOMING_INTERVAL;

    if (nevents > INCOMING_TCP_MAX)
        nevents = INCOMING_TCP_MAX;

    statCounter.tcp.comm_incoming.count(nevents);
}

/* poll all sockets; call handlers for those that are ready. */
Comm::Flag
Comm::DoSelect(int msec)
{
    struct pollfd pfds[SQUID_MAXFD];

    PF *hdl = nullptr;
    int fd;
    int maxfd;
    unsigned long nfds;
    unsigned long npending;
    int num;
    int calldns = 0, calludp = 0, calltcp = 0;
    double timeout = current_dtime + (msec / 1000.0);

    do {
        double start;
        getCurrentTime();
        start = current_dtime;

        if (commCheckUdpIncoming)
            comm_poll_udp_incoming();

        if (commCheckDnsIncoming)
            comm_poll_dns_incoming();

        if (commCheckTcpIncoming)
            comm_poll_tcp_incoming();

        calldns = calludp = calltcp = 0;

        nfds = 0;

        npending = 0;

        maxfd = Biggest_FD + 1;

        for (int i = 0; i < maxfd; ++i) {
            int events;
            events = 0;
            /* Check each open socket for a handler. */

            if (fd_table[i].read_handler)
                events |= POLLRDNORM;

            if (fd_table[i].write_handler)
                events |= POLLWRNORM;

            if (events) {
                pfds[nfds].fd = i;
                pfds[nfds].events = events;
                pfds[nfds].revents = 0;
                ++nfds;

                if ((events & POLLRDNORM) && fd_table[i].flags.read_pending)
                    ++npending;
            }
        }

        if (npending)
            msec = 0;

        if (msec > MAX_POLL_TIME)
            msec = MAX_POLL_TIME;

        /* nothing to do
         *
         * Note that this will only ever trigger when there are no log files
         * and stdout/err/in are all closed too.
         */
        if (nfds == 0 && npending == 0) {
            if (shutting_down)
                return Comm::SHUTDOWN;
            else
                return Comm::IDLE;
        }

        for (;;) {
            ++ statCounter.syscalls.selects;
            num = poll(pfds, nfds, msec);
            int xerrno = errno;
            ++ statCounter.select_loops;

            if (num >= 0 || npending > 0)
                break;

            if (ignoreErrno(xerrno))
                continue;

            debugs(5, DBG_CRITICAL, MYNAME << "poll failure: " << xstrerr(xerrno));

            assert(xerrno != EINVAL);

            return Comm::COMM_ERROR;

            /* NOTREACHED */
        }

        getCurrentTime();

        debugs(5, num ? 5 : 8, "comm_poll: " << num << "+" << npending << " FDs ready");
        statCounter.select_fds_hist.count(num);

        if (num == 0 && npending == 0)
            continue;

        /* scan each socket but the accept socket. Poll this
         * more frequently to minimize losses due to the 5 connect
         * limit in SunOS */

        for (size_t loopIndex = 0; loopIndex < nfds; ++loopIndex) {
            fde *F;
            int revents = pfds[loopIndex].revents;
            fd = pfds[loopIndex].fd;

            if (fd == -1)
                continue;

            if (fd_table[fd].flags.read_pending)
                revents |= POLLIN;

            if (revents == 0)
                continue;

            if (fdIsUdpListen(fd)) {
                calludp = 1;
                continue;
            }

            if (fdIsDns(fd)) {
                calldns = 1;
                continue;
            }

            if (fdIsTcpListen(fd)) {
                calltcp = 1;
                continue;
            }

            F = &fd_table[fd];

            if (revents & (POLLRDNORM | POLLIN | POLLHUP | POLLERR)) {
                debugs(5, 6, "comm_poll: FD " << fd << " ready for reading");

                if ((hdl = F->read_handler)) {
                    F->read_handler = nullptr;
                    hdl(fd, F->read_data);
                    ++ statCounter.select_fds;

                    if (commCheckUdpIncoming)
                        comm_poll_udp_incoming();

                    if (commCheckDnsIncoming)
                        comm_poll_dns_incoming();

                    if (commCheckTcpIncoming)
                        comm_poll_tcp_incoming();
                }
            }

            if (revents & (POLLWRNORM | POLLOUT | POLLHUP | POLLERR)) {
                debugs(5, 6, "comm_poll: FD " << fd << " ready for writing");

                if ((hdl = F->write_handler)) {
                    F->write_handler = nullptr;
                    hdl(fd, F->write_data);
                    ++ statCounter.select_fds;

                    if (commCheckUdpIncoming)
                        comm_poll_udp_incoming();

                    if (commCheckDnsIncoming)
                        comm_poll_dns_incoming();

                    if (commCheckTcpIncoming)
                        comm_poll_tcp_incoming();
                }
            }

            if (revents & POLLNVAL) {
                AsyncCall::Pointer ch;
                debugs(5, DBG_CRITICAL, "WARNING: FD " << fd << " has handlers, but it's invalid.");
                debugs(5, DBG_CRITICAL, "FD " << fd << " is a " << fdTypeStr[F->type]);
                debugs(5, DBG_CRITICAL, "--> " << F->desc);
                debugs(5, DBG_CRITICAL, "tmout:" << F->timeoutHandler << "read:" <<
                       F->read_handler << " write:" << F->write_handler);

                for (ch = F->closeHandler; ch != nullptr; ch = ch->Next())
                    debugs(5, DBG_CRITICAL, " close handler: " << ch);

                if (F->closeHandler != nullptr) {
                    commCallCloseHandlers(fd);
                } else if (F->timeoutHandler != nullptr) {
                    debugs(5, DBG_CRITICAL, "comm_poll: Calling Timeout Handler");
                    ScheduleCallHere(F->timeoutHandler);
                }

                F->closeHandler = nullptr;
                F->timeoutHandler = nullptr;
                F->read_handler = nullptr;
                F->write_handler = nullptr;

                if (F->flags.open)
                    fd_close(fd);
            }
        }

        if (calludp)
            comm_poll_udp_incoming();

        if (calldns)
            comm_poll_dns_incoming();

        if (calltcp)
            comm_poll_tcp_incoming();

        getCurrentTime();

        statCounter.select_time += (current_dtime - start);

        return Comm::OK;
    } while (timeout > current_dtime);

    debugs(5, 8, "comm_poll: time out: " << squid_curtime << ".");

    return Comm::TIMEOUT;
}

static void
comm_poll_dns_incoming(void)
{
    int nfds = 0;
    int fds[2];
    int nevents;
    statCounter.dns.ioEvents = 0;

    if (DnsSocketA < 0 && DnsSocketB < 0)
        return;

    if (DnsSocketA >= 0) {
        fds[nfds] = DnsSocketA;
        ++nfds;
    }

    if (DnsSocketB >= 0) {
        fds[nfds] = DnsSocketB;
        ++nfds;
    }

    nevents = comm_check_incoming_poll_handlers(nfds, fds);

    if (nevents < 0)
        return;

    statCounter.dns.interval += Config.comm_incoming.dns.average - nevents;

    if (statCounter.dns.interval < Config.comm_incoming.dns.min_poll)
        statCounter.dns.interval = Config.comm_incoming.dns.min_poll;

    if (statCounter.dns.interval > MAX_INCOMING_INTERVAL)
        statCounter.dns.interval = MAX_INCOMING_INTERVAL;

    if (nevents > INCOMING_DNS_MAX)
        nevents = INCOMING_DNS_MAX;

    statCounter.dns.comm_incoming.count(nevents);
}

void
Comm::SelectLoopInit(void)
{
}

/* Called by async-io or diskd to speed up the polling */
void
Comm::QuickPollRequired(void)
{
    MAX_POLL_TIME = 10;
}

#endif /* USE_POLL */

