/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_COMM_SOCKSACCEPTOR_H
#define SQUID_COMM_SOCKSACCEPTOR_H

#include "comm/TcpAcceptor.h"

namespace Comm
{

/**
 * Listens on a Comm::Connection for new incoming SOCKS v5 connections
 * and emits an active Comm::Connection descriptor for the new client.
 *
 * \see Comm::TcpAcceptor
 */
class SocksAcceptor : public TcpAcceptor
{
    CBDATA_CLASS(SocksAcceptor);

public:
    typedef CbcPointer<Comm::SocksAcceptor> Pointer;

private:
    virtual void start() override;
    virtual bool doneAll() const override;
    virtual void swanSong() override;
    virtual const char *status() const override;

    SocksAcceptor(const SocksAcceptor &); // not implemented.

public:
    SocksAcceptor(const Comm::ConnectionPointer &conn, const char *note, const Subscription::Pointer &aSub);
    SocksAcceptor(const AnyP::PortCfgPointer &listenPort, const char *note, const Subscription::Pointer &aSub);

    /* Comm::TcpAcceptor API */
    virtual Comm::Flag oldAccept(Comm::ConnectionPointer &) override;
    virtual bool setListen() override;
};

} // namespace Comm

#endif /* SQUID_COMM_SOCKSACCEPTOR_H */

