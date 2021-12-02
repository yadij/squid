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
    virtual ~ACLExtUser();

    /* ACL API */
    virtual char const *typeString() const override { return type_; }
    virtual void parse() override { data->parse(); }
    virtual void parseFlags() override;
    virtual int match(ACLChecklist *) override;
    virtual SBufList dump() const override { return data->dump(); }
    virtual bool empty() const override { return data->empty(); }

private:
    ACLData<char const *> *data = nullptr;
    char const *type_ = nullptr;
};

#endif /* USE_AUTH */
#endif /* SQUID_EXTUSER_H */

