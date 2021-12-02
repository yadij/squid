/*
 * Copyright (C) 1996-2021 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLPROTOCOLDATA_H
#define SQUID_ACLPROTOCOLDATA_H

#include "acl/Acl.h"
#include "acl/Data.h"
#include "anyp/ProtocolType.h"

#include <list>

class ACLProtocolData : public ACLData<AnyP::ProtocolType>
{
    MEMPROXY_CLASS(ACLProtocolData);

public:
    virtual ~ACLProtocolData() {}

    /* ACLData API */
    virtual bool match(AnyP::ProtocolType) override;
    virtual SBufList dump() const override;
    virtual void parse() override;
    virtual bool empty() const override { return values.empty(); }
    virtual ACLData<AnyP::ProtocolType> *clone() const override;

public:
    std::list<AnyP::ProtocolType> values;
};

#endif /* SQUID_ACLPROTOCOLDATA_H */

