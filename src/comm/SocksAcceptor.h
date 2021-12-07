/*
 * Copyright (C) 1996-2021 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef _SQUID__SRC_COMM_SOCKSACCEPTOR_H
#define _SQUID__SRC_COMM_SOCKSACCEPTOR_H

#if USE_SOCKS

#include "comm/TcpAcceptor.h"

namespace Comm
{

/**
 * Listens for new incoming SOCKS connections.
 * \see Comm::TcpAcceptor
 */
class SocksAcceptor : public Comm::TcpAcceptor
{
    CBDATA_CHILD(SocksAcceptor);

public:
    SocksAcceptor(const Comm::ConnectionPointer &c, const char *note, const Subscription::Pointer &s) :
        TcpAcceptor(c, note, s)
    {}
    SocksAcceptor(const AnyP::PortCfgPointer &p, const char *note, const Subscription::Pointer &s) :
        TcpAcceptor(p, note, s)
    {}
    virtual ~SocksAcceptor() {}

    /* Comm::TcpAcceptor API */
    virtual Comm::Flag oldAccept(Comm::ConnectionPointer &) override;
    virtual bool setListen() override;
};

} // namespace Comm

#endif /* USE_SOCKS */
#endif /* _SQUID__SRC_COMM_SOCKSACCEPTOR_H */

