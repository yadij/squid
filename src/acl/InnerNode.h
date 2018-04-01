/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACL_INNER_NODE_H
#define SQUID_ACL_INNER_NODE_H

#include "acl/MatchNode.h"

#include <vector>

namespace Acl
{

typedef std::vector<Acl::MatchNodePointer> Nodes; ///< a collection of nodes

/// An intermediate ACL tree node. Manages a collection of child tree nodes.
class InnerNode: public Acl::MatchNode
{
public:
    // No ~InnerNode() to delete children. They are aclRegister()ed instead.

    /// Resumes matching (suspended by an async call) at the given position.
    bool resumeMatchingAt(ACLChecklist *checklist, Acl::Nodes::const_iterator pos) const;

    /// the number of children nodes
    Nodes::size_type childrenCount() const { return nodes.size(); }

    /// parses one "acl name type acl1 acl2..." line, appending to nodes
    void lineParse();

    /// appends the node to the collection and takes control over it
    void add(const Acl::MatchNodePointer &node);

    /* Acl::MatchNode API */
    virtual SBufList dump() const override;
    virtual bool empty() const override;
    virtual void prepareForUse() override;

protected:
    /* Acl::MatchNode API */
    virtual int match(ACLChecklist *) override;

protected:
    /// checks whether the nodes match, starting with the given one
    /// kids determine what a match means for their type of intermediate nodes
    virtual int doMatch(ACLChecklist *checklist, Nodes::const_iterator start) const = 0;

    Nodes nodes; ///< children nodes of this intermediate node
};

} // namespace Acl

#endif /* SQUID_ACL_INNER_NODE_H */

