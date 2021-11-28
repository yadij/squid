/*
 * Copyright (C) 1996-2021 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLMETHODDATA_H
#define SQUID_ACLMETHODDATA_H

#include "acl/Acl.h"
#include "acl/Data.h"
#include "http/RequestMethod.h"

#include <list>

class ACLMethodData : public ACLData<HttpRequestMethod>
{
    MEMPROXY_CLASS(ACLMethodData);

public:
    virtual ~ACLMethodData() {}

    /* ACLData API */
    virtual bool match(HttpRequestMethod);
    virtual SBufList dump() const;
    virtual void parse();
    virtual bool empty() const { return values.empty(); }
    virtual ACLData<HttpRequestMethod> *clone() const;

public:
    static int ThePurgeCount; ///< PURGE methods seen by parse()

    std::list<HttpRequestMethod> values;
};

#endif /* SQUID_ACLMETHODDATA_H */

