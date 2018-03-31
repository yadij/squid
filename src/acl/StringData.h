/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLSTRINGDATA_H
#define SQUID_ACLSTRINGDATA_H

#include "acl/Data.h"
#include "sbuf/SBuf.h"

#include <set>

class ACLStringData : public ACLData<char const *>
{
    MEMPROXY_CLASS(ACLStringData);

public:
    ACLStringData() {}
    ACLStringData(ACLStringData const &);
    ACLStringData &operator= (ACLStringData const &);
    virtual ~ACLStringData() {}

    bool match(const SBuf &);
    /// Insert a string data value
    void insert(const char *);

    /* ACLData<T> API */
    virtual bool match(const char *) override; ///< \deprecated use match(SBuf&) instead
    virtual SBufList dump() const override;
    virtual void parse() override;
    virtual ACLData *clone() const override;
    virtual bool empty() const override;

private:
    typedef std::set<SBuf> StringValues_t;
    StringValues_t stringValues;
};

#endif /* SQUID_ACLSTRINGDATA_H */

