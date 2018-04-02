/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACL_TRANSACTION_INITIATOR_H
#define SQUID_ACL_TRANSACTION_INITIATOR_H

#include "acl/Checklist.h"
#include "acl/MatchNode.h"
#include "XactionInitiator.h"

namespace Acl
{

/// transaction_initiator ACL
class TransactionInitiator : public Acl::MatchNode
{
    MEMPROXY_CLASS(TransactionInitiator);

public:
    TransactionInitiator(char const *);

    /* Acl::MatchNode API */
    virtual void parse() override;
    virtual char const *typeString() const override;
    virtual SBufList dump() const override;
    virtual bool empty() const override;
    virtual int match(ACLChecklist *) override;
    virtual bool requiresRequest() const override { return true; }

protected:
    char const *class_;
    XactionInitiator::Initiators initiators_;
    SBufList cfgWords; /// initiator names in the configured order
};

} // namespace Acl

#endif /* SQUID_ACL_TRANSACTION_INITIATOR_H */

