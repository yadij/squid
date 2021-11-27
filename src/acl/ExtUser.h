/*
 * Copyright (C) 1996-2021 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_EXTUSER_H
#define SQUID_EXTUSER_H

#if USE_AUTH

#include "acl/Acl.h"
#include "acl/Checklist.h"
#include "acl/Data.h"

class ACLExtUser : public ACL
{
    MEMPROXY_CLASS(ACLExtUser);

public:
    ACLExtUser(ACLData<char const *> *, char const *);
    ACLExtUser(ACLExtUser const &);
    virtual ~ACLExtUser();

    ACLExtUser &operator =(ACLExtUser const &);

    /* ACL API */
    virtual char const *typeString() const;
    virtual void parse();
    virtual void parseFlags();
    virtual int match(ACLChecklist *checklist);
    virtual SBufList dump() const;
    virtual bool empty () const;
    virtual ACL *clone()const;

private:
    ACLData<char const *> *data = nullptr;
    char const *type_ = nullptr;
};

#endif /* USE_AUTH */
#endif /* SQUID_EXTUSER_H */

