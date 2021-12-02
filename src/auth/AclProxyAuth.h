/*
 * Copyright (C) 1996-2021 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLPROXYAUTH_H
#define SQUID_ACLPROXYAUTH_H

#if USE_AUTH

#include "acl/Acl.h"
#include "acl/Checklist.h"
#include "acl/Data.h"

class ProxyAuthLookup : public ACLChecklist::AsyncState
{

public:
    static ProxyAuthLookup *Instance();
    virtual void checkForAsync(ACLChecklist *) const;

private:
    static ProxyAuthLookup instance_;
    static void LookupDone(void *data);
};

class ACLProxyAuth : public ACL
{
    MEMPROXY_CLASS(ACLProxyAuth);

public:
    ACLProxyAuth(ACLData<char const *> *, char const *);
    virtual ~ACLProxyAuth();

    /* ACL API */
    virtual char const *typeString() const override { return type_; }
    virtual void parse() override { data->parse(); }
    virtual bool isProxyAuth() const override { return true; }
    virtual void parseFlags() override;
    virtual int match(ACLChecklist *) override;
    virtual SBufList dump() const override { return data->dump(); }
    virtual bool valid() const override;
    virtual bool empty() const override { return data->empty(); }
    virtual bool requiresRequest() const override { return true; }
    virtual int matchForCache(ACLChecklist *) override;

private:
    int matchProxyAuth(ACLChecklist *);

    ACLData<char const *> *data = nullptr;
    char const *type_ = nullptr;
};

#endif /* USE_AUTH */
#endif /* SQUID_ACLPROXYAUTH_H */

