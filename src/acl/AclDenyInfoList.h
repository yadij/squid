/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLDENYINFOLIST_H_
#define SQUID_ACLDENYINFOLIST_H_

#include "acl/forward.h"
#include "err_type.h"
#include "errorpage.h"
#include "mem/forward.h"

namespace Acl
{

/// deny_info representation
class DenyInfoList : public RefCountable
{
    MEMPROXY_CLASS(Acl::DenyInfoList);

public:
    DenyInfoList(const char *t) {
        err_page_name = xstrdup(t);
        err_page_id = errorReservePageId(t);
    }
    ~DenyInfoList() {
        xfree(err_page_name);
        while (next) {
            auto *a = next;
            next = a->next;
            a->next = nullptr;
            delete a;
        }
    }
    err_type err_page_id = ERR_NONE;
    char *err_page_name = nullptr;
    SBufList acl_list; ///< ACL names in configured order
    DenyInfoList *next = nullptr;
};

} // namespace Acl

#endif /* SQUID_ACLDENYINFOLIST_H_ */

