/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_EXTERNALACL_H
#define SQUID_EXTERNALACL_H

#include "acl/Checklist.h"
#include "acl/MatchNode.h"
#include "base/RefCount.h"

class external_acl;
class external_acl_data;
class StoreEntry;

class ExternalACLLookup : public ACLChecklist::AsyncState
{

public:
    static ExternalACLLookup *Instance();
    virtual void checkForAsync(ACLChecklist *)const;

    // If possible, starts an asynchronous lookup of an external ACL.
    // Otherwise, asserts (or bails if background refresh is requested).
    static void Start(ACLChecklist *checklist, external_acl_data *acl, bool bg);

private:
    static ExternalACLLookup instance_;
    static void LookupDone(void *data, const ExternalACLEntryPointer &result);
};

class ACLExternal : public Acl::MatchNode
{
    MEMPROXY_CLASS(ACLExternal);

public:
    static void ExternalAclLookup(ACLChecklist * ch, ACLExternal *);

    ACLExternal(char const *);
    ACLExternal(ACLExternal const &);
    virtual ~ACLExternal();
    ACLExternal&operator=(ACLExternal const &);

    virtual Acl::MatchNode *clone() const;

    /* Acl::MatchNode API */
    virtual void parse() override;
    virtual char const *typeString() const override;
    virtual bool isProxyAuth() const override;
    virtual SBufList dump() const override;
    virtual bool empty() const override;
    virtual bool valid() const override;
    virtual int match(ACLChecklist *) override;
    /* requires*() really should be dynamic based on the external class defn */
    virtual bool requiresAle() const override { return true; }
    virtual bool requiresRequest() const override { return true; }

protected:
    external_acl_data *data;
    char const *class_;
};

void parse_externalAclHelper(external_acl **);
void dump_externalAclHelper(StoreEntry * sentry, const char *name, const external_acl *);
void free_externalAclHelper(external_acl **);
typedef void EAH(void *data, const ExternalACLEntryPointer &result);
void externalAclLookup(ACLChecklist * ch, void *acl_data, EAH * handler, void *data);
void externalAclInit(void);
void externalAclShutdown(void);

#endif /* SQUID_EXTERNALACL_H */

