/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_IDENT_ACLIDENT_H
#define SQUID_IDENT_ACLIDENT_H

#if USE_IDENT
#include "acl/Checklist.h"
#include "acl/Data.h"
#include "acl/MatchNode.h"

/// \ingroup ACLAPI
class IdentLookup : public ACLChecklist::AsyncState
{

public:
    static IdentLookup *Instance();
    virtual void checkForAsync(ACLChecklist *)const;

private:
    static IdentLookup instance_;
    static void LookupDone(const char *ident, void *data);
};

/// \ingroup ACLAPI
class ACLIdent : public Acl::MatchNode
{
    MEMPROXY_CLASS(ACLIdent);

public:
    ACLIdent(ACLData<char const *> *newData, char const *);
    ACLIdent (ACLIdent const &old);
    ACLIdent & operator= (ACLIdent const &rhs);
    ~ACLIdent();

    /* Acl::MatchNode API */
    virtual char const *typeString() const;
    virtual void parse();
    virtual bool isProxyAuth() const {return true;}
    virtual void parseFlags();
    virtual int match(ACLChecklist *checklist);
    virtual SBufList dump() const;
    virtual bool empty () const;
    virtual Acl::MatchNode *clone() const;

private:
    ACLData<char const *> *data;
    char const *type_;
};

#endif /* USE_IDENT */
#endif /* SQUID_IDENT_ACLIDENT_H */

