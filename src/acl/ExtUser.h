/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_EXTUSER_H
#define SQUID_EXTUSER_H

#if USE_AUTH

#include "acl/Checklist.h"
#include "acl/Data.h"
#include "acl/MatchNode.h"

class ACLExtUser : public Acl::MatchNode
{
    MEMPROXY_CLASS(ACLExtUser);

public:
    ACLExtUser(ACLData<char const *> *newData, char const *);
    ACLExtUser (ACLExtUser const &old);
    ACLExtUser & operator= (ACLExtUser const &rhs);
    virtual ~ACLExtUser();

    virtual Acl::MatchNode *clone() const;

    /* Acl::MatchNode API */
    virtual void parseFlags() override;
    virtual void parse() override;
    virtual char const *typeString() const override;
    virtual SBufList dump() const override;
    virtual bool empty() const override;
    virtual int match(ACLChecklist *) override;

private:
    ACLData<char const *> *data;
    char const *type_;
};

#endif /* USE_AUTH */
#endif /* SQUID_EXTUSER_H */

