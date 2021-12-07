/*
 * Copyright (C) 1996-2021 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 05    SOCKS Listener Socket Handler */

#include "squid.h"

#if USE_SOCKS

#include "anyp/PortCfg.h"
#include "comm/SocksAcceptor.h"
#include "fd.h"
#include "fde.h"
#include "StatCounters.h"

#include <cerrno>

#if HAVE_SOCKS_H
#include <socks.h>
#endif

CBDATA_NAMESPACED_CLASS_INIT(Comm, SocksAcceptor);

bool
Comm::SocksAcceptor::setListen()
{
    errcode = errno = 0;
    if (Rlisten(listeningConnection()->fd, Squid_MaxFD >> 2) < 0) {
        errcode = errno;
        debugs(50, DBG_CRITICAL, "ERROR: Rlisten(..., " << (Squid_MaxFD >> 2) << ") system call failed: " << xstrerr(errcode));
        return false;
    }
    return true;
}

/**
 * accept() and process
 * Wait for an incoming connection on our listener socket.
 *
 * \retval Comm::OK          success. details parameter filled.
 * \retval Comm::NOMESSAGE   attempted accept() but nothing useful came in.
 *                           Or this client has too many connections already.
 * \retval Comm::COMM_ERROR  an outright failure occurred.
 */
Comm::Flag
Comm::SocksAcceptor::oldAccept(Comm::ConnectionPointer &details)
{
    ++statCounter.syscalls.sock.accepts;
    int sock;
    struct addrinfo *gai = NULL;
    Ip::Address::InitAddr(gai);

    errcode = 0; // reset local errno copy.
    if ((sock = Raccept(listeningConnection()->fd, gai->ai_addr, &gai->ai_addrlen)) < 0) {
        errcode = errno; // store last accept errno locally.

        Ip::Address::FreeAddr(gai);

        if (ignoreErrno(errcode) || errcode == ECONNABORTED) {
            debugs(50, 5, status() << ": " << xstrerr(errcode));
            return Comm::NOMESSAGE;
        } else if (errcode == ENFILE || errcode == EMFILE) {
            debugs(50, 3, status() << ": " << xstrerr(errcode));
            return Comm::COMM_ERROR;
        } else {
            debugs(50, DBG_IMPORTANT, "ERROR: failed to Raccept an incoming connection: " << xstrerr(errcode));
            return Comm::COMM_ERROR;
        }
    }

    Must(sock >= 0);
    ++incoming_sockets_accepted;

    debugs(50, 2, "SOCKS ACCEPT: incoming connection: FD " << sock << " socks_isfd=" << (socks_isfd(sock)?"T":"F"));
    fd_open(sock, FD_SOCKET, "SOCKS Client");

    details->fd = sock;
    details->flags |= (socks_isfd(sock)?COMM_SOCKS:0);
    details->remote = *gai;

    // lookup the local-end details of this new connection
    Ip::Address::InitAddr(gai);
    details->local.setEmpty();
    if (Rgetsockname(sock, gai->ai_addr, &gai->ai_addrlen) != 0) {
        int xerrno = errno;
        debugs(50, DBG_IMPORTANT, "ERROR: Rgetsockname() failed to locate local-IP on " << details << ": " << xstrerr(xerrno));
        Ip::Address::FreeAddr(gai);
        return Comm::COMM_ERROR;
    }
    details->local = *gai;
    Ip::Address::FreeAddr(gai);

    return Comm::OK;
}

#endif /* USE_SOCKS */

