/*
 * Copyright (C) 1996-2022 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 27    Cache Announcer */

#include "squid.h"
#include "anyp/PortCfg.h"
#include "base/RunnersRegistry.h"
#include "comm/Connection.h"
#include "event.h"
#include "fd.h"
#include "fde.h"
#include "fs_io.h"
#include "globals.h"
#include "ICP.h"
#include "ipcache.h"
#include "SquidConfig.h"
#include "tools.h"

static void
send_announce(const ipcache_addrs *ia, const Dns::LookupDetails &, void *)
{
    LOCAL_ARRAY(char, tbuf, 256);
    LOCAL_ARRAY(char, sndbuf, BUFSIZ);

    char *host = Config.Announce.host;
    char *file = nullptr;
    unsigned short port = Config.Announce.port;
    int l;
    int n;
    int fd;

    if (ia == nullptr) {
        debugs(27, DBG_IMPORTANT, "ERROR: send_announce: Unknown host '" << host << "'");
        return;
    }

    debugs(27, DBG_IMPORTANT, "Sending Announcement to " << host);
    sndbuf[0] = '\0';
    snprintf(tbuf, 256, "cache_version SQUID/%s\n", version_string);
    strcat(sndbuf, tbuf);
    assert(HttpPortList != nullptr);
    snprintf(tbuf, 256, "Running on %s %d %d\n",
             getMyHostname(),
             getMyPort(),
             (int) Config.Port.icp);
    strcat(sndbuf, tbuf);

    if (Config.adminEmail) {
        snprintf(tbuf, 256, "cache_admin: %s\n", Config.adminEmail);
        strcat(sndbuf, tbuf);
    }

    snprintf(tbuf, 256, "generated %d [%s]\n",
             (int) squid_curtime,
             Time::FormatHttpd(squid_curtime));
    strcat(sndbuf, tbuf);
    l = strlen(sndbuf);

    if ((file = Config.Announce.file) != nullptr) {
        fd = file_open(file, O_RDONLY | O_TEXT);

        if (fd > -1 && (n = FD_READ_METHOD(fd, sndbuf + l, BUFSIZ - l - 1)) > 0) {
            fd_bytes(fd, n, FD_READ);
            l += n;
            sndbuf[l] = '\0';
            file_close(fd);
        } else {
            int xerrno = errno;
            debugs(50, DBG_IMPORTANT, "send_announce: " << file << ": " << xstrerr(xerrno));
        }
    }

    Ip::Address S = ia->current();
    S.port(port);
    assert(Comm::IsConnOpen(icpOutgoingConn));

    if (comm_udp_sendto(icpOutgoingConn->fd, S, sndbuf, strlen(sndbuf) + 1) < 0) {
        int xerrno = errno;
        debugs(27, DBG_IMPORTANT, "ERROR: Failed to announce to " << S << " from " << icpOutgoingConn->local << ": " << xstrerr(xerrno));
    }
}

static void
start_announce(void *)
{
    if (!Config.onoff.announce)
        return;

    if (!Comm::IsConnOpen(icpOutgoingConn))
        return;

    ipcache_nbgethostbyname(Config.Announce.host, send_announce, nullptr);

    eventAdd("send_announce", start_announce, nullptr, static_cast<double>(Config.Announce.period), 1);
}

/// launches send-announce event
class SendAnnounceRr: public RegisteredRunner
{
public:
    /* RegisteredRunner API */
    virtual void finalizeConfig() {
        eventDelete(start_announce, nullptr);
        if (Config.onoff.announce)
            eventAdd("send_announce", start_announce, nullptr, Config.Announce.period, 1);
    }

    virtual void startShutdown() {
        eventDelete(start_announce, nullptr);
    }
};

RunnerRegistrationEntry(SendAnnounceRr);
