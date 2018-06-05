/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef _SQUID_COMPAT_SOCKS_H
#define _SQUID_COMPAT_SOCKS_H

/* wrapper for socks.h to undo global socket API replacement
 * and lack of extern "C" API protections.
 */

#if HAVE_SOCKS_H
#include <sys/types.h>
#include <sys/socket.h>
/*
 * The definition of bindresvport below might conflict with
 * <netinet/in.h> ... best workaround seems to be to make sure the
 * file is included prior to the #define
 */
#include <netinet/in.h>
#include <netdb.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <socks.h>
#ifdef __cplusplus
}
#endif

#undef accept
#undef bind
#undef bindresvport
#undef connect
#undef gethostbyname
#undef gethostbyname2
#undef getaddrinfo
#undef getipnodebyname
#undef getpeername
#undef getsockname
#undef getsockopt
#undef listen
#undef read
#undef readv
#undef recv
#undef recvfrom
#undef recvmsg
#undef rresvport
#undef send
#undef sendmsg
#undef sendto
#undef write
#undef writev

#endif /* HAVE_SOCKS_H */
#endif /* _SQUID_COMPAT_SOCKS_H */
