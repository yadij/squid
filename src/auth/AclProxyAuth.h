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
    ACLProxyAuth(ACLData<char const *> *, char const *);
    ACLProxyAuth(ACLProxyAuth const &);
    ACLProxyAuth &operator =(ACLProxyAuth const &);

    /* Acl::MatchNode API */
    virtual char const *typeString() const;
    virtual void parse();
    virtual bool isProxyAuth() const {return true;}
    virtual void parseFlags();
    virtual int match(ACLChecklist *checklist);
    virtual SBufList dump() const;
    virtual bool valid() const;
    virtual bool empty() const;
    virtual bool requiresRequest() const {return true;}
    virtual Acl::MatchNode *clone() const;
    virtual int matchForCache(ACLChecklist *checklist);

private:
    int matchProxyAuth(ACLChecklist *);
    ACLData<char const *> *data;
    char const *type_;
};

#endif /* USE_AUTH */
#endif /* SQUID_ACLPROXYAUTH_H */

