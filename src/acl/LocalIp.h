/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLLOCALIP_H
#define SQUID_ACLLOCALIP_H

#include "acl/Ip.h"

/// \ingroup ACLAPI
class ACLLocalIP : public ACLIP
{
    MEMPROXY_CLASS(ACLLocalIP);

public:
    virtual Acl::MatchNode *clone() const;

    /* ACLIP API */
    virtual char const *typeString() const override;
    virtual int match(ACLChecklist *) override;
};

#endif /* SQUID_ACLLOCALIP_H */

