/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLMAXUSERIP_H
#define SQUID_ACLMAXUSERIP_H

#if USE_AUTH

#include "acl/MatchNode.h"
#include "auth/UserRequest.h"

class ACLMaxUserIP : public Acl::MatchNode
{
    MEMPROXY_CLASS(ACLMaxUserIP);

public:
    explicit ACLMaxUserIP(char const *theClass);

    int getMaximum() const { return maximum; }

    /* Acl::MatchNode API */
    virtual const Acl::Options &options() override;
    virtual void parse() override;
    virtual char const *typeString() const override;
    virtual SBufList dump() const override;
    virtual bool empty() const override;
    virtual bool valid() const override;
    virtual int match(ACLChecklist *) override;
    virtual bool requiresRequest() const override { return true; }

private:
    int match(Auth::UserRequest::Pointer, Ip::Address const &);

public:
    Acl::BooleanOptionValue beStrict; ///< Enforce "one user, one device" policy?

private:
    char const *class_;
    int maximum;
};

#endif /* USE_AUTH */
#endif /* SQUID_ACLMAXUSERIP_H */

