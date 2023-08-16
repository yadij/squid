/*
 * Copyright (C) 1996-2023 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_SRC_MASTERXACTION_H
#define SQUID_SRC_MASTERXACTION_H

#include "anyp/forward.h"
#include "anyp/PortCfg.h"
#include "base/InstanceId.h"
#include "base/Lock.h"
#include "base/RefCount.h"
#include "comm/forward.h"
#include "XactionInitiator.h"

/** Master transaction details.
 *
 * Aggregates historical data from individual related protocol-specific
 * transactions such as an HTTP client transaction and the corresponding
 * HTTP or FTP server transaction.
 *
 * Individual transaction information worth sending or logging should be
 * recorded here, ideally without exposing other master transaction users
 * to internal details of individual transactions. For example, storing an
 * HTTP client IP address is a good idea but storing a pointer to some
 * client-side job which maintains that address is not.
 *
 * A master transaction is created by a newly accepted client connection,
 * a new request on the existing client connection, or an internal request
 * generated by Squid. All client-side protocols, including HTTP, HTCP, ICP,
 * and SNMP will eventually create master transactions.
 *
 * A master transaction is auto-destroyed when its last user is gone.
 */
class MasterXaction : public RefCountable
{
public:
    typedef RefCount<MasterXaction> Pointer;

    /// Create a master transaction not associated with a AnyP::PortCfg port.
    template <XactionInitiator::Initiator anInitiator>
    static Pointer MakePortless()
    {
        static_assert(anInitiator != XactionInitiator::initClient, "not an HTTP or FTP client");
        return Pointer::Make(anInitiator, nullptr);
    }

    /// Create a master transaction associated with a AnyP::PortCfg port.
    /// \param aPort may be nil if port information was lost
    static Pointer MakePortful(const AnyP::PortCfgPointer &aPort)
    {
        return Pointer::Make(XactionInitiator::initClient, aPort);
    }

    /// transaction ID.
    InstanceId<MasterXaction, uint64_t> id;

    /// the listening port which originated this transaction
    AnyP::PortCfgPointer squidPort;

    /// the client TCP connection which originated this transaction
    Comm::ConnectionPointer tcpClient;

    /// the initiator of this transaction
    XactionInitiator initiator;

    /// whether we are currently creating a CONNECT header (to be sent to peer)
    bool generatingConnect = false;

    // TODO: add state from other Jobs in the transaction

private:
    friend class RefCount<MasterXaction>;

    // use public Make() functions instead
    MasterXaction(const XactionInitiator anInitiator, const AnyP::PortCfgPointer &aPort):
        squidPort(aPort),
        initiator(anInitiator)
    {}
};

#endif /* SQUID_SRC_MASTERXACTION_H */

