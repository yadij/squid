/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACL_H
#define SQUID_ACL_H

#include "acl/forward.h"
#include "acl/Options.h"
#include "cbdata.h"
#include "defines.h"
#include "dlink.h"
#include "sbuf/forward.h"

#include <algorithm>
#include <ostream>

class ConfigParser;

namespace Acl {

/// the ACL type name known to admins
typedef const char *TypeName;
/// a "factory" function for making ACL objects (of some ACL child type)
typedef ACL *(*Maker)(TypeName typeName);
/// use the given ACL Maker for all ACLs of the named type
void RegisterMaker(TypeName typeName, Maker maker);

} // namespace Acl

/// A configurable condition. A node in the ACL expression tree.
/// Can evaluate itself in FilledChecklist context.
/// Does not change during evaluation.
/// \ingroup ACLAPI
class ACL
{

public:
    void *operator new(size_t);
    void operator delete(void *);

    static void ParseAclLine(ConfigParser &parser, ACL ** head);
    static void Initialize();
    static ACL *FindByName(const char *name);

    ACL();
    virtual ~ACL();

    /// sets user-specified ACL name and squid.conf context
    void context(const char *name, const char *configuration);

    /// Orchestrates matching checklist against the ACL using match(),
    /// after checking preconditions and while providing debugging.
    /// \return true if and only if there was a successful match.
    /// Updates the checklist state on match, async, and failure.
    bool matches(ACLChecklist *checklist) const;

    /// \returns (linked) Options supported by this ACL
    virtual const Acl::Options &options() { return Acl::NoOptions(); }

    /// configures ACL options, throwing on configuration errors
    virtual void parseFlags();

    /// parses node represenation in squid.conf; dies on failures
    virtual void parse() = 0;
    virtual char const *typeString() const = 0;
    virtual bool isProxyAuth() const;
    virtual SBufList dump() const = 0;
    virtual bool empty() const = 0;
    virtual bool valid() const;

    int cacheMatchAcl(dlink_list * cache, ACLChecklist *);
    virtual int matchForCache(ACLChecklist *checklist);

    virtual void prepareForUse() {}

    SBufList dumpOptions(); ///< \returns approximate options configuration

    char name[ACL_NAME_SZ];
    char *cfgline;
    ACL *next; // XXX: remove or at least use refcounting
    bool registered; ///< added to the global list of ACLs via aclRegister()

private:
    /// Matches the actual data in checklist against this ACL.
    virtual int match(ACLChecklist *checklist) = 0; // XXX: missing const

    /// whether our (i.e. shallow) match() requires checklist to have a AccessLogEntry
    virtual bool requiresAle() const;
    /// whether our (i.e. shallow) match() requires checklist to have a request
    virtual bool requiresRequest() const;
    /// whether our (i.e. shallow) match() requires checklist to have a reply
    virtual bool requiresReply() const;
};

/// \ingroup ACLAPI
class acl_proxy_auth_match_cache
{
    MEMPROXY_CLASS(acl_proxy_auth_match_cache);

public:
    acl_proxy_auth_match_cache(int matchRv, void * aclData) :
        matchrv(matchRv),
        acl_data(aclData)
    {}

    dlink_node link;
    int matchrv;
    void *acl_data;
};

/// \ingroup ACLAPI
/// XXX: find a way to remove or at least use a refcounted ACL pointer
extern const char *AclMatchedName;  /* NULL */

#endif /* SQUID_ACL_H */

