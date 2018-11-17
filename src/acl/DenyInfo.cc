/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/*
 * DEBUG: section 28    Access Control
 */

#include "squid.h"
#include "acl/DenyInfo.h"
#include "SquidConfig.h"
#include "Store.h"

/* maex@space.net (05.09.96)
 *    get the info for redirecting "access denied" to info pages
 *      TODO (probably ;-)
 *      currently there is no optimization for
 *      - more than one deny_info line with the same url
 *      - a check, whether the given acl really is defined
 *      - a check, whether an acl is added more than once for the same url
 */
void
Acl::DenyInfo::ParseConfigLine()
{
    char *t = NULL;

    /* first expect a page name */

    if ((t = ConfigParser::NextToken()) == NULL) {
        debugs(28, DBG_CRITICAL, cfg_filename << " line " << config_lineno << ": " << config_input_line);
        debugs(28, DBG_CRITICAL, "ERROR: deny_info missing 'error page' parameter, skipping.");
        return;
    }

    Acl::DenyInfoPointer A(new Acl::DenyInfo(t));

    /* next expect a list of ACL names */
    while ((t = ConfigParser::NextToken())) {
        A->acl_list.emplace_back(t);
    }

    if (A->acl_list.empty()) {
        debugs(28, DBG_CRITICAL, cfg_filename << " line " << config_lineno << ": " << config_input_line);
        debugs(28, DBG_CRITICAL, "ERROR: deny_info line contains no ACL's, skipping");
        return;
    }

    Config.denyInfo.emplace_back(A);
}

/* does name lookup, returns page_id */
err_type
Acl::DenyInfo::FindByAclName(const char *name, bool redirectAllowed)
{
    if (!name) {
        debugs(28, 3, "ERR_NONE due to a nil name");
        return ERR_NONE;
    }

    debugs(28, 8, "lookup for " << name);

    for (const auto &itr : Config.denyInfo) {
        if (!redirectAllowed && strchr(itr->err_page_name, ':') ) {
            // BUG: this also catches explicit 4xx status responses
            debugs(28, 8, "skip '" << itr->err_page_name << "' 30x redirects not allowed as response here");
            continue;
        }

        for (const auto &aclName: itr->acl_list) {
            if (aclName.cmp(name) == 0) {
                debugs(28, 8, "match on " << name);
                return itr->err_page_id;
            }
        }
    }

    debugs(28, 8, "no match for " << name);
    return ERR_NONE;
}

void
dump_denyinfo(StoreEntry * entry, const char *name, const Acl::DenyInfoList &list)
{
    for (const auto &itr : list) {
        storeAppendPrintf(entry, "%s %s", name, itr->err_page_name);

        for (const auto &aclName: itr->acl_list)
            storeAppendPrintf(entry, " " SQUIDSBUFPH, SQUIDSBUFPRINT(aclName));

        storeAppendPrintf(entry, "\n");
    }
}

