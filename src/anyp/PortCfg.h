/*
 * Copyright (C) 1996-2021 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ANYP_PORTCFG_H
#define SQUID_ANYP_PORTCFG_H

#include "anyp/forward.h"
#include "anyp/ProtocolVersion.h"
#include "anyp/TrafficMode.h"
#include "base/CodeContext.h"
#include "comm/Connection.h"
#include "comm/Tcp.h"
#include "sbuf/SBuf.h"
#include "security/ServerOptions.h"

namespace AnyP
{

class PortCfg : public CodeContext
{
public:
    PortCfg();
    ~PortCfg();
    AnyP::PortCfgPointer clone() const;

    /* CodeContext API */
    virtual ScopedId codeContextGist() const override;
    virtual std::ostream &detailCodeContext(std::ostream &os) const override;

    PortCfgPointer next;

    Ip::Address s;
    AnyP::ProtocolVersion transport; ///< transport protocol and version received by this port
    char *name = nullptr; ///< visible name
    char *defaultsite = nullptr; ///< default web site

    TrafficMode flags; ///< flags indicating what type of traffic to expect via this port.

    bool allow_direct = false; ///< Allow direct forwarding in accelerator mode
    bool vhost = false; ///< uses host header
    bool actAsOrigin = false; ///< update replies to conform with RFC 2616
    bool ignore_cc = false; ///< Ignore request Cache-Control directives
    bool connection_auth_disabled = false; ///< Don't support connection oriented auth
    bool ftp_track_dirs = false; ///< whether transactions should track FTP directories

    int vport = 0; ///< virtual port support. -1 if dynamic, >0 static
    int disable_pmtu_discovery = 0;
    bool workerQueues = false; ///< whether listening queues should be worker-specific

    Comm::TcpKeepAlive tcp_keepalive;

    /**
     * The listening socket details.
     * If Comm::ConnIsOpen() we are actively listening for client requests.
     * use listenConn->close() to stop.
     */
    Comm::ConnectionPointer listenConn;

    /// TLS configuration options for this listening port
    Security::ServerOptions secure;
};

} // namespace AnyP

/// list of Squid http(s)_port configured
extern AnyP::PortCfgPointer HttpPortList;

/// list of Squid ftp_port configured
extern AnyP::PortCfgPointer FtpPortList;

#if !defined(MAXTCPLISTENPORTS)
// Max number of TCP listening ports
#define MAXTCPLISTENPORTS 128
#endif

// TODO: kill this global array. Need to check performance of array vs list though.
extern int NHttpSockets;
extern int HttpSockets[MAXTCPLISTENPORTS];

#endif /* SQUID_ANYP_PORTCFG_H */

