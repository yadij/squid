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
#include "Store.h"

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

