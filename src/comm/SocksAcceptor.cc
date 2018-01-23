/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 05    SOCKSv5 Listener Socket Handler */

#include "squid.h"

#if USE_SOCKS
#include "anyp/PortCfg.h"
#include "client_db.h"
#include "comm/Connection.h"
#include "comm/comm_internal.h"
#include "comm/SocksAcceptor.h"
#include "CommCalls.h"
#include "eui/Config.h"
#include "fd.h"
#include "fde.h"
#include "ip/Intercept.h"
#include "profiler/Profiler.h"
#include "SquidConfig.h"
#include "StatCounters.h"

#include <cerrno>
#if HAVE_SOCKS_H
#include <socks.h>
#endif

CBDATA_NAMESPACED_CLASS_INIT(Comm, SocksAcceptor);

Comm::SocksAcceptor::SocksAcceptor(const Comm::ConnectionPointer &newConn, const char *, const Subscription::Pointer &aSub) :
    Comm::TcpAcceptor(newConn, nullptr, aSub)
{}

Comm::SocksAcceptor::SocksAcceptor(const AnyP::PortCfgPointer &p, const char *, const Subscription::Pointer &aSub) :
    Comm::TcpAcceptor(p, nullptr, aSub)
{}

/**
 * New-style listen and accept routines
 *
 * setListen simply registers our interest in an FD for listening.
 * The constructor takes a callback to call when an FD has been
 * Raccept()ed some time later.
 */
bool
Comm::SocksAcceptor::setListen()
{
    errcode = errno = 0;
    if (Rlisten(getConn()->fd, Squid_MaxFD >> 2) < 0) {
        errcode = errno;
        debugs(50, DBG_CRITICAL, "ERROR: Rlisten(" << status() << ", " << (Squid_MaxFD >> 2) << "): " << xstrerr(errcode));
        return false;
    }

    fd_open(getConn()->fd, FD_SOCKET, "SOCKS Listener");
    return true;
}

/**
 * Raccept() and process
 * Wait for an incoming connection on our listener socket.
 *
 * \retval Comm::OK         success. details parameter filled.
 * \retval Comm::NOMESSAGE  attempted Raccept() but nothing useful came in.
 * \retval Comm::COMM_ERROR      an outright failure occured.
 *                         Or if this client has too many connections already.
 */
Comm::Flag
Comm::SocksAcceptor::oldAccept(Comm::ConnectionPointer &details)
{
    PROF_start(comm_accept);
    ++statCounter.syscalls.sock.accepts;
    int sock;
    struct addrinfo *gai = NULL;
    Ip::Address::InitAddr(gai);

    errcode = 0; // reset local errno copy.
    if ((sock = Raccept(getConn()->fd, gai->ai_addr, &gai->ai_addrlen)) < 0) {
        errcode = errno; // store last accept errno locally.

        Ip::Address::FreeAddr(gai);

        PROF_stop(comm_accept);

        if (ignoreErrno(errcode)) {
            debugs(50, 5, status() << ": " << xstrerr(errcode));
            return Comm::NOMESSAGE;
        } else if (ENFILE == errno || EMFILE == errno) {
            debugs(50, 3, status() << ": " << xstrerr(errcode));
            return Comm::COMM_ERROR;
        } else {
            debugs(50, DBG_IMPORTANT, MYNAME << status() << ": " << xstrerr(errcode));
            return Comm::COMM_ERROR;
        }
    }

    Must(sock >= 0);
    details->fd = sock;
    details->remote = *gai;

    if ( Config.client_ip_max_connections >= 0) {
        if (clientdbEstablished(details->remote, 0) > Config.client_ip_max_connections) {
            debugs(50, DBG_IMPORTANT, "WARNING: " << details->remote << " attempting more than " << Config.client_ip_max_connections << " connections.");
            Ip::Address::FreeAddr(gai);
            PROF_stop(comm_accept);
            return Comm::COMM_ERROR;
        }
    }

    // lookup the local-end details of this new connection
    Ip::Address::InitAddr(gai);
    details->local.setEmpty();
    if (Rgetsockname(sock, gai->ai_addr, &gai->ai_addrlen) != 0) {
        int xerrno = errno;
        debugs(50, DBG_IMPORTANT, "ERROR: getsockname() failed to locate local-IP on " << details << ": " << xstrerr(xerrno));
        Ip::Address::FreeAddr(gai);
        PROF_stop(comm_accept);
        return Comm::COMM_ERROR;
    }
    details->local = *gai;
    Ip::Address::FreeAddr(gai);

    /* fdstat update */
    // XXX : these are not all HTTP requests. use a note about type and ip:port details->
    // so we end up with a uniform "(HTTP|FTP-data|HTTPS|...) remote-ip:remote-port"
    fd_open(sock, FD_SOCKET, "HTTP Request");

    fde *F = &fd_table[sock];
    details->remote.toStr(F->ipaddr,MAX_IPSTRLEN);
    F->remote_port = details->remote.port();
    F->local_addr = details->local;
    F->sock_family = details->local.isIPv6()?AF_INET6:AF_INET;

    // set socket flags
    commSetCloseOnExec(sock);
    commSetNonBlocking(sock);

    /* IFF the socket is (tproxy) transparent, pass the flag down to allow spoofing */
    F->flags.transparent = fd_table[getConn()->fd].flags.transparent; // XXX: can we remove this line yet?

    // Perform NAT or TPROXY operations to retrieve the real client/dest IP addresses
    if (getConn()->flags&(COMM_TRANSPARENT|COMM_INTERCEPTION) && !Ip::Interceptor.Lookup(details, getConn())) {
        debugs(50, DBG_IMPORTANT, "ERROR: NAT/TPROXY lookup failed to locate original IPs on " << details);
        // Failed.
        PROF_stop(comm_accept);
        return Comm::COMM_ERROR;
    }

#if USE_SQUID_EUI
    if (Eui::TheConfig.euiLookup) {
        if (details->remote.isIPv4()) {
            details->remoteEui48.lookup(details->remote);
        } else if (details->remote.isIPv6()) {
            details->remoteEui64.lookup(details->remote);
        }
    }
#endif

    PROF_stop(comm_accept);
    return Comm::OK;
}

#endif /* USE_SOCKS */

