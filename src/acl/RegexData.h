/*
 * Copyright (C) 1996-2021 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLREGEXDATA_H
#define SQUID_ACLREGEXDATA_H

#include "acl/Data.h"
#include "base/RegexPattern.h"

#include <list>

class ACLRegexData : public ACLData<char const *>
{
    MEMPROXY_CLASS(ACLRegexData);

public:
    virtual ~ACLRegexData() {}

    /* ACLData API */
    virtual bool match(char const *) override;
    virtual SBufList dump() const override;
    virtual void parse() override;
    virtual const Acl::ParameterFlags &supportedFlags() const override;
    virtual bool empty() const override { return data.empty(); }
    virtual ACLData<char const *> *clone() const override;

private:
    std::list<RegexPattern> data;
};

#endif /* SQUID_ACLREGEXDATA_H */

