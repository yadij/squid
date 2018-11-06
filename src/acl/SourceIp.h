/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLSOURCEIP_H
#define SQUID_ACLSOURCEIP_H

#include "acl/Ip.h"

class ACLSourceIP : public ACLIP
{
    MEMPROXY_CLASS(ACLSourceIP);

public:
    /* ACLIP API */
    virtual char const *typeString() const override;
    virtual int match(ACLChecklist *) override;
};

#endif /* SQUID_ACLSOURCEIP_H */

