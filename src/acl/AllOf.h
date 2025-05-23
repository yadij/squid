/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_SRC_ACL_ALLOF_H
#define SQUID_SRC_ACL_ALLOF_H

#include "acl/InnerNode.h"

namespace Acl
{

/// Configurable all-of ACL. Each ACL line is a conjunction of ACLs.
/// Uses AndNode and OrNode to handle squid.conf configuration where multiple
/// acl all-of lines are always ORed together.
class AllOf: public Acl::InnerNode
{
    MEMPROXY_CLASS(AllOf);

public:
    /* Acl::Node API */
    char const *typeString() const override;
    void parse() override;
    SBufList dump() const override;

private:
    /* Acl::InnerNode API */
    int doMatch(ACLChecklist *checklist, Nodes::const_iterator start) const override;
};

} // namespace Acl

#endif /* SQUID_SRC_ACL_ALLOF_H */

