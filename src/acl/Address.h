/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef _SQUID_SRC_ACL_ADDRESS_H
#define _SQUID_SRC_ACL_ADDRESS_H

#include "acl/forward.h"
#include "cbdata.h"
#include "ip/Address.h"

namespace Acl
{

/// list of address-based ACLs.
class Address
{
    CBDATA_CLASS(Address);

public:
    Address() : next(NULL) {}
    ~Address();

    Acl::Address *next;
    ACLListPointer aclList;

    Ip::Address addr;
};

} // namespace Acl

#endif /* _SQUID_SRC_ACL_ADDRESS_H */

