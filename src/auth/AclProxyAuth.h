/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLPROXYAUTH_H
#define SQUID_ACLPROXYAUTH_H

#if USE_AUTH

#include "acl/Checklist.h"
#include "acl/Data.h"
#include "acl/MatchNode.h"

class ProxyAuthLookup : public ACLChecklist::AsyncState
{

public:
    static ProxyAuthLookup *Instance();
    virtual void checkForAsync(ACLChecklist *) const;

private:
    static ProxyAuthLookup instance_;
    static void LookupDone(void *data);
};

class ACLProxyAuth : public Acl::MatchNode
{
    MEMPROXY_CLASS(ACLProxyAuth);

public:
    ~ACLProxyAuth();
    ACLProxyAuth(Acl::Data<char const *> *, char const *);
    ACLProxyAuth(ACLProxyAuth const &);
    ACLProxyAuth &operator =(ACLProxyAuth const &);

    /* Acl::MatchNode API */
    virtual void parseFlags() override;
    virtual void parse() override;
    virtual char const *typeString() const override;
    virtual bool isProxyAuth() const override { return true; }
    virtual SBufList dump() const override;
    virtual bool empty() const override;
    virtual bool valid() const override;
    virtual int matchForCache(ACLChecklist *) override;
    virtual int match(ACLChecklist *) override;
    virtual bool requiresRequest() const override { return true; }

private:
    int matchProxyAuth(ACLChecklist *);
    Acl::Data<char const *> *data;
    char const *type_;
};

#endif /* USE_AUTH */
#endif /* SQUID_ACLPROXYAUTH_H */

