/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_SRC_SNMP_MIBTREENODE_H
#define SQUID_SRC_SNMP_MIBTREENODE_H

#include "mem/forward.h"
#include "snmp/forward.h"
#include "snmp_core.h"

#include <vector>

namespace Snmp
{

class MibTreeNode : public RefCountable
{
    MEMPROXY_CLASS(Snmp::MibTreeNode);
public:
    MibTreeNode(oid *aName, size_t aLen, AggrType type) : name(aName), len(aLen), aggrType(type) {}

    /// become the parent node for the given sub-tree
    void addChild(const MibTreePointer &);

    /// search this sub-tree for the given OID string representation
    MibTreePointer findByName(const char *) const;

private:
    MibTreePointer findByOid(const oid *, const size_t) const;

public:
    oid *name = nullptr;
    const size_t len = 0;
    oid_ParseFn *parsefunction = {};
    instance_Fn *instancefunction = {};

    std::vector<MibTreePointer> leaves;
    MibTreePointer parent;
    AggrType aggrType = atNone;
};

} // namespace Snmp

#endif /* SQUID_SRC_SNMP_MIBTREENODE_H */
