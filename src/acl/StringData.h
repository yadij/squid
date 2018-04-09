/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_SRC_ACL_STRINGDATA_H
#define SQUID_SRC_ACL_STRINGDATA_H

#include "acl/Data.h"
#include "sbuf/SBuf.h"

#include <set>

namespace Acl {

class StringData : public ACLData<char const *>
{
    MEMPROXY_CLASS(StringData);

public:
    StringData() {}
    StringData(StringData const &);
    StringData &operator= (StringData const &);
    virtual ~StringData() {}

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

} // namespace Acl

#endif /* SQUID_SRC_ACL_STRINGDATA_H */

