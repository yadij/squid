/*
 * Copyright (C) 1996-2021 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLSIZELIMIT_H_
#define SQUID_ACLSIZELIMIT_H_

#include "acl/forward.h"
#include "cbdata.h"

/// representation of a class of Size-limit ACLs
class AclSizeLimit
{
    CBDATA_CLASS(AclSizeLimit);

public:
    ~AclSizeLimit();

    AclSizeLimit *next = nullptr;
    ACLList *aclList = nullptr;
    int64_t size = 0;
};

#endif /* SQUID_ACLSIZELIMIT_H_ */

