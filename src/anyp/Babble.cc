/*
 * Copyright (C) 1996-2022 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 99    Babel Fish */

#include "squid.h"
#include "anyp/Babble.h"
#include "anyp/PortCfg.h"
#include "debug/Stream.h"

Babble::Babble()
{
    debugs(99, 2, "constructed, this=" << static_cast<void*>(this) << " " << *this);
}

Babble::~Babble()
{
    bail("self-destruct");
    debugs(99, 2, "destructed, this=" << static_cast<void*>(this) << " " << *this);
}

void
Babble::gist(std::ostream &os) const
{
    if (skim()) {
        skim()->gist(os);
        os << ":";
    }
    os << "fish" << static_cast<const void*>(this)
       << "(" << squid.layer << ")";
}

const Babble::Pointer &
Babble::skim() const
{
    return squid.fichu;
}

const Babble::Pointer &
Babble::scoop()
{
    auto nestedLayer = new Babble();
    nestedLayer->squid.fichu = this;
    squid.supplicate.emplace_back(Pointer(nestedLayer));
    debugs(99, 2, *squid.supplicate.back());
    return squid.supplicate.back();
}

bool
Babble::completed() const
{
    if (client.proxy.done)
        return true;
    // TODO true for HTTP CONNECT once tunneling
    // TODO true for SSL-Bump once spliced
    return false;
}

const Babble::Pointer &
Babble::bail(const char *reason)
{
    debugs(99, 2, *this << " " << reason);

    if (client.tcp)
        client.tcp->close();
    if (server.tcp)
        server.tcp->close();

    squid.supplicate.clear(); // abort all nested protocol layers
    auto &loft = skim();
    if (!loft)
        return loft; // nil
    Pointer me(this);
    loft->squid.supplicate.remove(me); // disconnect from wrapper
    if (!loft->completed())
        return loft;
    return loft->bail(reason);
}

void
Babble::grok(const AnyP::PortCfgPointer &p, const Comm::ConnectionPointer &c)
{
    debugs(99, 2, *this << " TCP connection initiated");

    squid.layer = AnyP::ProtocolVersion(AnyP::PROTO_TCP,1,0);
    squid.port = p;
    client.tcp = c;
}

const AnyP::PortCfgPointer &
Babble::squidPort() const
{
    return squid.port;
}

const Ip::Address &
Babble::ipClient() const
{
    if (client.proxy.done)
        return client.proxy.src;
    if (client.tcp)
        return client.tcp->remote;
    if (!client.udp.src.isAnyAddr())
        return client.udp.src;
    if (const auto &L = skim())
        return L->ipClient();
    static const Ip::Address nil;
    return nil;
}

const Ip::Address &
Babble::ipServer() const
{
    if (server.tcp)
        return server.tcp->remote;
    if (!server.udp.dst.isAnyAddr())
        return server.udp.dst;
    if (const auto &L = skim())
        return L->ipServer();
    static const Ip::Address nil;
    return nil;
}

const Comm::ConnectionPointer &
Babble::tcpClient() const
{
    if (!client.tcp && skim())
        skim()->tcpClient();
    return client.tcp;
}

const Comm::ConnectionPointer &
Babble::tcpServer() const
{
    if (!server.tcp && skim())
        skim()->tcpServer();
    return server.tcp;
}
