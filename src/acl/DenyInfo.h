/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_SRC_ACL_DENYINFO_H
#define SQUID_SRC_ACL_DENYINFO_H

#include "acl/AclNameList.h"
#include "errorpage.h"
#include "sbuf/SBuf.h"

class Packable;

namespace Acl {

/// deny_info representation
class DenyInfo : public RefCountable
{
    MEMPROXY_CLASS(Acl::DenyInfo);

public:
    DenyInfo(const char *t) : pageName(t) {
        err_page_id = errorReservePageId(t);
    }
    ~DenyInfo() {
        delete acl_list;
    }

    void dump(Packable *) const;

    /// does name lookup, returns page_id
    static err_type GetPageId(const Acl::DenyInfoList &, const char *name, bool redirect_allowed);

    /// get the info for redirecting "access denied" to info pages
    static void ParseLine(Acl::DenyInfoList &);

private:
    err_type err_page_id = ERR_NONE;
    SBuf pageName;
    AclNameList *acl_list = nullptr;
};

} // namespace Acl

/// \deprecated wrapper for legacy config parser
inline void
parse_denyinfo(Acl::DenyInfoList *var)
{
    Acl::DenyInfo::ParseLine(*var);
}

/// \deprecated wrapper for legacy config parser
inline void
free_denyinfo(Acl::DenyInfoList *list)
{
    list->clear();
}

/// \deprecated wrapper for legacy config parser
void dump_denyinfo(Packable *entry, const char *name, const Acl::DenyInfoList &);

#endif /* SQUID_ACLDENYINFOLIST_H_ */

