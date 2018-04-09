/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_SRC_ACL_DATA_H
#define SQUID_SRC_ACL_DATA_H

#include "acl/Options.h"
#include "sbuf/List.h"

namespace Acl {

/// Configured ACL parameter(s) (e.g., domain names in dstdomain ACL).
template <class M>
class Data
{
public:
    virtual ~Data() {}

    /// \returns the flags supported by these ACL parameters (e.g., "-i")
    virtual const Acl::ParameterFlags &supportedFlags() const { return Acl::NoFlags(); }
    virtual bool match(M) =0;
    virtual SBufList dump() const =0;
    virtual void parse() =0;
    virtual Data *clone() const =0;
    virtual void prepareForUse() {}
    virtual bool empty() const =0;
};

} // namespace Acl

#endif /* SQUID_SRC_ACL_DATA_H */

