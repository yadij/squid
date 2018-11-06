/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACL_ANY_OF_H
#define SQUID_ACL_ANY_OF_H

#include "acl/BoolOps.h"

namespace Acl
{

/// Configurable any-of ACL. Each ACL line is a disjuction of ACLs.
class AnyOf: public Acl::OrNode
{
    MEMPROXY_CLASS(AnyOf);

public:
    virtual Acl::MatchNode *clone() const;

    /* Acl::OrNode API */
    virtual void parse() override;
    virtual char const *typeString() const override;
};

} // namespace Acl

#endif /* SQUID_ACL_ANY_OF_H */

