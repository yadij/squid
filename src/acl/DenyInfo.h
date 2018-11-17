/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_SRC_ACL_DENYINFO_H_
#define SQUID_SRC_ACL_DENYINFO_H_

#include "acl/forward.h"
#include "err_type.h"
#include "errorpage.h"
#include "mem/forward.h"

namespace Acl
{

/// deny_info representation
class DenyInfo : public RefCountable
{
    MEMPROXY_CLASS(Acl::DenyInfo);

public:
    DenyInfo(const char *t) {
        err_page_name = xstrdup(t);
        err_page_id = errorReservePageId(t);
    }
    ~DenyInfo() {
        xfree(err_page_name);
    }

    /// get the deny_info associated with the 'name' ACL
    static err_type FindByAclName(const char *name, bool redirectAllowed);

    /// parse a squid.conf deny_info line
    static void ParseConfigLine();

public:
    err_type err_page_id = ERR_NONE;
    char *err_page_name = nullptr;
    SBufList acl_list; ///< ACL names in configured order
};

} // namespace Acl

// wrappers for LegacyParser cf_gen API
#define parse_denyinfo(x) Acl::DenyInfo::ParseConfigLine()
#define free_denyinfo(list) (list)->clear()
void dump_denyinfo(StoreEntry *, const char *name, const Acl::DenyInfoList &);

#endif /* SQUID_SRC_ACL_DENYINFO_H_ */

