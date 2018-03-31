/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLMETHODDATA_H
#define SQUID_ACLMETHODDATA_H

#include "acl/Data.h"
#include "http/RequestMethod.h"

#include <list>

class ACLMethodData : public ACLData<HttpRequestMethod>
{
    MEMPROXY_CLASS(ACLMethodData);

public:
    ACLMethodData() {}
    ACLMethodData(ACLMethodData const &);
    ACLMethodData &operator= (ACLMethodData const &);
    virtual ~ACLMethodData();

    /* ACLData<T> API */
    virtual bool match(HttpRequestMethod) override;
    virtual SBufList dump() const override;
    virtual void parse() override;
    virtual ACLData *clone() const override;
    virtual bool empty() const override { return values.empty(); }

public:
    std::list<HttpRequestMethod> values;

    static int ThePurgeCount; ///< PURGE methods seen by parse()
};

#endif /* SQUID_ACLMETHODDATA_H */

