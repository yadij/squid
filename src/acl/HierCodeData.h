/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLHIERCODEDATA_H
#define SQUID_ACLHIERCODEDATA_H

#include "acl/Data.h"
#include "hier_code.h"

class ACLHierCodeData : public ACLData<hier_code>
{
    MEMPROXY_CLASS(ACLHierCodeData);

public:
    ACLHierCodeData();
    ACLHierCodeData(ACLHierCodeData const &);
    ACLHierCodeData &operator= (ACLHierCodeData const &);
    virtual ~ACLHierCodeData();

    /* ACLData<T> API */
    virtual bool match(hier_code) override;
    virtual SBufList dump() const override;
    virtual void parse() override;
    virtual ACLData *clone() const override;
    virtual bool empty() const override;

public:
    /// mask of codes this ACL might match.
    bool values[HIER_MAX];
};

#endif /* SQUID_ACLHIERCODEDATA_H */

