/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLPROTOCOLDATA_H
#define SQUID_ACLPROTOCOLDATA_H

#include "acl/Data.h"
#include "anyp/ProtocolType.h"

#include <list>

class ACLProtocolData : public Acl::Data<AnyP::ProtocolType>
{
    MEMPROXY_CLASS(ACLProtocolData);

public:
    ACLProtocolData() {}
    ACLProtocolData(ACLProtocolData const &);
    ACLProtocolData &operator= (ACLProtocolData const &);
    virtual ~ACLProtocolData();

    /* Acl::Data<T> API */
    virtual bool match(AnyP::ProtocolType) override;
    virtual SBufList dump() const override;
    virtual void parse() override;
    virtual Acl::Data<AnyP::ProtocolType> *clone() const override;
    virtual bool empty() const override { return values.empty(); }

public:
    std::list<AnyP::ProtocolType> values;
};

#endif /* SQUID_ACLPROTOCOLDATA_H */

