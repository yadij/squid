/*
 * Copyright (C) 1996-2021 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLSTRINGDATA_H
#define SQUID_ACLSTRINGDATA_H

#include "acl/Acl.h"
#include "acl/Data.h"
#include "sbuf/SBuf.h"

#include <set>

class ACLStringData : public ACLData<char const *>
{
    MEMPROXY_CLASS(ACLStringData);

public:
    virtual ~ACLStringData() {}

    /* ACLData API */
    /// \deprecated use match(SBuf&) instead.
    virtual bool match(char const *) override;
    virtual SBufList dump() const override;
    virtual void parse() override;
    virtual bool empty() const override { return stringValues.empty(); }
    virtual ACLData<char const *> *clone() const override;

    virtual bool match(const SBuf &);

    /// Insert a string data value
    void insert(const char *);

private:
    typedef std::set<SBuf> StringValues_t;
    StringValues_t stringValues;
};

#endif /* SQUID_ACLSTRINGDATA_H */

