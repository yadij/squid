/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_SRC_ACL_SIZELIMIT_H
#define SQUID_SRC_ACL_SIZELIMIT_H

#include "acl/forward.h"
#include "cbdata.h"

namespace Acl {

/// representation of a class of Size-limit ACLs
class SizeLimit
{
    CBDATA_CLASS(SizeLimit);

public:
    ACLListPointer aclList;
    int64_t size = 0;
};

} // namespace Acl

#endif /* SQUID_SRC_ACL_SIZELIMIT_H_ */

