/*
 * Copyright (C) 1996-2021 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLMAXCONNECTION_H
#define SQUID_ACLMAXCONNECTION_H

#include "acl/Acl.h"

/// \ingroup ACLAPI
class ACLMaxConnection : public ACL
{
    MEMPROXY_CLASS(ACLMaxConnection);

public:
    ACLMaxConnection(char const *);
    ACLMaxConnection(ACLMaxConnection const &) = default;
    virtual~ACLMaxConnection() {}

    /* ACl API */
    virtual ACL *clone() const;
    virtual char const *typeString() const { return class_; }
    virtual void parse();
    virtual int match(ACLChecklist *);
    virtual SBufList dump() const;
    virtual bool empty() const { return false; }
    virtual bool valid() const { return limit > 0; }
    virtual void prepareForUse();

protected:
    char const *class_ = nullptr;
    int limit = -1;
};

#endif /* SQUID_ACLMAXCONNECTION_H */

