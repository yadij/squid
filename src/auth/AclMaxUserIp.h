/*
 * Copyright (C) 1996-2021 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLMAXUSERIP_H
#define SQUID_ACLMAXUSERIP_H

#if USE_AUTH

#include "acl/Acl.h"
#include "auth/UserRequest.h"

class ACLMaxUserIP : public ACL
{
    MEMPROXY_CLASS(ACLMaxUserIP);

public:
    explicit ACLMaxUserIP(char const *theClass);
    virtual ~ACLMaxUserIP() {}

    /* ACL API */
    virtual const Acl::Options &options() override;
    virtual void parse() override;
    virtual char const *typeString() const override { return class_; }
    virtual SBufList dump() const override;
    virtual bool empty() const override { return false; }
    virtual bool valid() const override { return maximum > 0; }
    virtual int match(ACLChecklist *) override;
    virtual bool requiresRequest() const override { return true; }

    int getMaximum() const { return maximum; }

private:
    int match(Auth::UserRequest::Pointer auth_user_request, Ip::Address const &src_addr);

public:
    Acl::BooleanOptionValue beStrict; ///< Enforce "one user, one device" policy?

private:
    char const *class_ = nullptr;
    int maximum = 0;
};

#endif /* USE_AUTH */
#endif /* SQUID_ACLMAXUSERIP_H */

