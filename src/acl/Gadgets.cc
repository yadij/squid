/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/*
 * DEBUG: section 28    Access Control
 *
 * This file contains ACL routines that are not part of the
 * ACL class, nor any other class yet, and that need to be
 * factored into appropriate places. They are here to reduce
 * unneeded dependencies between the ACL class and the rest
 * of squid.
 */

#include "squid.h"
#include "acl/Acl.h"
#include "acl/Checklist.h"
#include "acl/Gadgets.h"
#include "acl/Strategised.h"
#include "acl/Tree.h"
#include "ConfigParser.h"
#include "errorpage.h"
#include "globals.h"
#include "HttpRequest.h"

#include <set>
#include <algorithm>

typedef std::set<ACL*> AclSet;
/// Accumulates all ACLs to facilitate their clean deletion despite reuse.
static AclSet *RegisteredAcls; // TODO: Remove when ACLs are refcounted

/* does name lookup, returns if it is a proxy_auth acl */
int
aclIsProxyAuth(const char *name)
{
    if (!name) {
        debugs(28, 3, "false due to a NULL name");
        return false;
    }

    debugs(28, 5, "aclIsProxyAuth: called for " << name);

    ACL *a;

    if ((a = ACL::FindByName(name))) {
        debugs(28, 5, "aclIsProxyAuth: returning " << a->isProxyAuth());
        return a->isProxyAuth();
    }

    debugs(28, 3, "aclIsProxyAuth: WARNING, called for nonexistent ACL");
    return false;
}

void
aclParseAccessLine(const char *directive, ConfigParser &, acl_access **treep)
{
    /* first expect either 'allow' or 'deny' */
    const char *t = ConfigParser::NextToken();

    if (!t) {
        debugs(28, DBG_CRITICAL, "aclParseAccessLine: " << cfg_filename << " line " << config_lineno << ": " << config_input_line);
        debugs(28, DBG_CRITICAL, "aclParseAccessLine: missing 'allow' or 'deny'.");
        return;
    }

    allow_t action = ACCESS_DUNNO;
    if (!strcmp(t, "allow"))
        action = ACCESS_ALLOWED;
    else if (!strcmp(t, "deny"))
        action = ACCESS_DENIED;
    else {
        debugs(28, DBG_CRITICAL, "aclParseAccessLine: " << cfg_filename << " line " << config_lineno << ": " << config_input_line);
        debugs(28, DBG_CRITICAL, "aclParseAccessLine: expecting 'allow' or 'deny', got '" << t << "'.");
        return;
    }

    const int ruleId = ((treep && *treep) ? (*treep)->childrenCount() : 0) + 1;
    MemBuf ctxBuf;
    ctxBuf.init();
    ctxBuf.appendf("%s#%d", directive, ruleId);
    ctxBuf.terminate();

    Acl::AndNode *rule = new Acl::AndNode;
    rule->context(ctxBuf.content(), config_input_line);
    rule->lineParse();
    if (rule->empty()) {
        debugs(28, DBG_CRITICAL, "aclParseAccessLine: " << cfg_filename << " line " << config_lineno << ": " << config_input_line);
        debugs(28, DBG_CRITICAL, "aclParseAccessLine: Access line contains no ACL's, skipping");
        delete rule;
        return;
    }

    /* Append to the end of this list */

    assert(treep);
    if (!*treep) {
        *treep = new Acl::Tree;
        (*treep)->context(directive, config_input_line);
    }

    (*treep)->add(rule, action);

    /* We lock _acl_access structures in ACLChecklist::matchNonBlocking() */
}

// aclParseAclList does not expect or set actions (cf. aclParseAccessLine)
void
aclParseAclList(ConfigParser &, Acl::Tree **treep, const char *label)
{
    // accomodate callers unable to convert their ACL list context to string
    if (!label)
        label = "...";

    MemBuf ctxLine;
    ctxLine.init();
    ctxLine.appendf("(%s %s line)", cfg_directive, label);
    ctxLine.terminate();

    Acl::AndNode *rule = new Acl::AndNode;
    rule->context(ctxLine.content(), config_input_line);
    rule->lineParse();

    MemBuf ctxTree;
    ctxTree.init();
    ctxTree.appendf("%s %s", cfg_directive, label);
    ctxTree.terminate();

    // We want a cbdata-protected Tree (despite giving it only one child node).
    Acl::Tree *tree = new Acl::Tree;
    tree->add(rule);
    tree->context(ctxTree.content(), config_input_line);

    assert(treep);
    assert(!*treep);
    *treep = tree;
}

void
aclRegister(ACL *acl)
{
    if (!acl->registered) {
        if (!RegisteredAcls)
            RegisteredAcls = new AclSet;
        RegisteredAcls->insert(acl);
        acl->registered = true;
    }
}

/// remove registered acl from the centralized deletion set
static
void
aclDeregister(ACL *acl)
{
    if (acl->registered) {
        if (RegisteredAcls)
            RegisteredAcls->erase(acl);
        acl->registered = false;
    }
}

/*********************/
/* Destroy functions */
/*********************/

/// called to delete ALL Acls.
void
aclDestroyAcls(ACL ** head)
{
    *head = NULL; // Config.aclList
    if (AclSet *acls = RegisteredAcls) {
        debugs(28, 8, "deleting all " << acls->size() << " ACLs");
        while (!acls->empty()) {
            ACL *acl = *acls->begin();
            // We use centralized deletion (this function) so ~ACL should not
            // delete other ACLs, but we still deregister first to prevent any
            // accesses to the being-deleted ACL via RegisteredAcls.
            assert(acl->registered); // make sure we are making progress
            aclDeregister(acl);
            delete acl;
        }
    }
}

void
aclDestroyAclList(ACLList **list)
{
    debugs(28, 8, "aclDestroyAclList: invoked");
    assert(list);
    delete *list;
    *list = NULL;
}

void
aclDestroyAccessList(acl_access ** list)
{
    assert(list);
    if (*list)
        debugs(28, 3, "destroying: " << *list << ' ' << (*list)->name);
    delete *list;
    *list = NULL;
}

