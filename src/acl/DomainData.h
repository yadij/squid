/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLDOMAINDATA_H
#define SQUID_ACLDOMAINDATA_H

#include "acl/Data.h"
#include "splay.h"

class ACLDomainData : public ACLData<char const *>
{
    MEMPROXY_CLASS(ACLDomainData);

public:
    ACLDomainData() : domains(nullptr) {}
    virtual ~ACLDomainData();

    /* ACLData<T> API */
    virtual bool match(char const *) override;
    virtual SBufList dump() const override;
    virtual void parse() override;
    virtual ACLData *clone() const override;
    virtual bool empty() const override;

public:
    Splay<char *> *domains;
};

#endif /* SQUID_ACLDOMAINDATA_H */

