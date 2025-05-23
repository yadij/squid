/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "acl/Acl.h"
#include "acl/BoolOps.h"
#include "acl/Checklist.h"
#include "acl/Gadgets.h"
#include "acl/InnerNode.h"
#include "cache_cf.h"
#include "ConfigParser.h"
#include "debug/Stream.h"
#include "globals.h"
#include "sbuf/SBuf.h"
#include <algorithm>

void
Acl::InnerNode::prepareForUse()
{
    for (auto node : nodes)
        node->prepareForUse();
}

bool
Acl::InnerNode::empty() const
{
    return nodes.empty();
}

void
Acl::InnerNode::add(Acl::Node *node)
{
    assert(node != nullptr);
    nodes.push_back(node);
}

// kids use this method to handle [multiple] parse() calls correctly
size_t
Acl::InnerNode::lineParse()
{
    // XXX: not precise, may change when looping or parsing multiple lines
    if (!cfgline)
        cfgline = xstrdup(config_input_line);

    // expect a list of ACL names, each possibly preceded by '!' for negation

    size_t count = 0;
    while (const char *t = ConfigParser::strtokFile()) {
        const bool negated = (*t == '!');
        if (negated)
            ++t;

        debugs(28, 3, "looking for ACL " << t);
        // XXX: Performance regression: SBuf will allocate memory.
        const auto a = Acl::Node::FindByName(SBuf(t));

        if (a == nullptr) {
            debugs(28, DBG_CRITICAL, "ERROR: ACL not found: " << t);
            self_destruct();
            return count; // not reached
        }

        // append(negated ? new NotNode(a) : a);
        if (negated)
            add(new NotNode(a));
        else
            add(a);

        ++count;
    }

    return count;
}

SBufList
Acl::InnerNode::dump() const
{
    SBufList rv;
    for (const auto &node: nodes)
        rv.push_back(node->name);
    return rv;
}

int
Acl::InnerNode::match(ACLChecklist *checklist)
{
    return doMatch(checklist, nodes.begin());
}

bool
Acl::InnerNode::resumeMatchingAt(ACLChecklist *checklist, Acl::Nodes::const_iterator pos) const
{
    debugs(28, 5, "checking " << name << " at " << (pos-nodes.begin()));
    const int result = doMatch(checklist, pos);
    const char *extra = checklist->asyncInProgress() ? " async" : "";
    debugs(28, 3, "checked: " << name << " = " << result << extra);

    // merges async and failures (-1) into "not matched"
    return result == 1;
}

