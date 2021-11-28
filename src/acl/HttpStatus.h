/*
 * Copyright (C) 1996-2021 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLHTTPSTATUS_H
#define SQUID_ACLHTTPSTATUS_H

#include "acl/Acl.h"
#include "acl/Checklist.h"
#include "splay.h"

class acl_httpstatus_data
{
public:
    acl_httpstatus_data(int);
    acl_httpstatus_data(int, int);

    static int compare(acl_httpstatus_data * const &, acl_httpstatus_data * const &);

    SBuf toStr() const;

    int status1 = 0;
    int status2 = 0;
};

/// \ingroup ACLAPI
class ACLHTTPStatus : public ACL
{
    MEMPROXY_CLASS(ACLHTTPStatus);

public:
    ACLHTTPStatus(char const *);
    ACLHTTPStatus(ACLHTTPStatus const &);
    ~ACLHTTPStatus();
    ACLHTTPStatus&operator=(ACLHTTPStatus const &) = delete;

    virtual ACL *clone()const;
    virtual char const *typeString() const;
    virtual void parse();
    virtual int match(ACLChecklist *checklist);
    virtual SBufList dump() const;
    virtual bool empty () const;
    virtual bool requiresReply() const { return true; }

protected:
    Splay<acl_httpstatus_data*> *data = nullptr;
    char const *class_ = nullptr;
};

#endif /* SQUID_ACLHTTPSTATUS_H */

