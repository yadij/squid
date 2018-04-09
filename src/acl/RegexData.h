/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLREGEXDATA_H
#define SQUID_ACLREGEXDATA_H

#include "acl/Data.h"

#include <list>

class RegexPattern;

class ACLRegexData : public Acl::Data<char const *>
{
    MEMPROXY_CLASS(ACLRegexData);

public:
    virtual ~ACLRegexData();

    /* Acl::Data<T> API */
    virtual const Acl::ParameterFlags &supportedFlags() const override;
    virtual bool match(const char *) override;
    virtual SBufList dump() const override;
    virtual void parse() override;
    virtual Acl::Data<char const *> *clone() const override;
    virtual bool empty() const override;

private:
    std::list<RegexPattern> data;
};

#endif /* SQUID_ACLREGEXDATA_H */

