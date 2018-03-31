/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLMAXCONNECTION_H
#define SQUID_ACLMAXCONNECTION_H

#include "acl/MatchNode.h"

/// \ingroup ACLAPI
class ACLMaxConnection : public Acl::MatchNode
{
    MEMPROXY_CLASS(ACLMaxConnection);

public:
    ACLMaxConnection(char const *);
    ACLMaxConnection(ACLMaxConnection const &);
    virtual ~ACLMaxConnection();
    ACLMaxConnection&operator=(ACLMaxConnection const &);

    virtual Acl::MatchNode *clone() const;

    /* Acl::MatchNode API */
    virtual void parse() override;
    virtual char const *typeString() const override;
    virtual SBufList dump() const override;
    virtual bool empty() const override;
    virtual bool valid() const override;
    virtual void prepareForUse() override;
    virtual int match(ACLChecklist *) override;

protected:
    char const *class_;
    int limit;
};

#endif /* SQUID_ACLMAXCONNECTION_H */

