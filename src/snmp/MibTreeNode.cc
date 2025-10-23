/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "snmp/MibTreeNode.h"

void
Snmp::MibTreeNode::addChild(const MibTreePointer &child)
{
    debugs(49, 5, "assigning " << child << " to parent " << this);
    leaves.emplace_back(child);
    leaves.back()->parent = this;
}

Snmp::MibTreePointer
Snmp::MibTreeNode::findByName(const char *str) const
{
    oid *wanted;
    size_t wantedLen;

    if (!snmpCreateOidFromStr(str, &wanted, &wantedLen))
        return nullptr;

    auto result = findByOid(wanted, wantedLen);
    xfree(wanted);
    return result;
}

Snmp::MibTreePointer
Snmp::MibTreeNode::findByOid(const oid *wanted, const size_t wantedLen) const
{
    /* I wish there were some kind of sensible existing tree traversal
     * routine to use. I'll worry about that later */
    if (wantedLen <= 1)
        return this;       /* XXX it should only be this? */

    // TODO: walking 'up' the tree is not needed yet.
    assert(wantedLen >= len);

    // check that we are a sub-tree of this OID
    if (memcmp(wanted, name, len) != 0)
        return nullptr;

    MibTreePointer e = this;
    size_t depth = len; // how deep into the OID we have matched so far

    while (depth < wantedLen) {

        /* Find the child node which matches this OID */
        MibTreePointer foundChild;
        for (const auto &i : e->leaves) {
            if (i->name[depth] != wanted[depth]) {
                foundChild = i;
                break;
            }
        }

        /* halt if/when none of the children match */
        if (!foundChild)
            break;

        /* walk down the found child's branch */
        e = foundChild;
        ++depth;
    }

    return e;
}
