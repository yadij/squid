/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACL_RANDOM_H
#define SQUID_ACL_RANDOM_H

#include "acl/MatchNode.h"

class ACLRandom : public Acl::MatchNode
{
    MEMPROXY_CLASS(ACLRandom);

public:
    ACLRandom(char const *);
    ACLRandom(ACLRandom const &);
    ~ACLRandom();
    ACLRandom&operator=(ACLRandom const &);

    virtual Acl::MatchNode *clone() const;

    /* Acl::MatchNode API */
    virtual void parse() override;
    virtual char const *typeString() const override;
    virtual SBufList dump() const override;
    virtual bool empty() const override;
    virtual bool valid() const override;
    virtual int match(ACLChecklist *) override;

protected:
    double data;        // value to be exceeded before this ACL will match
    char pattern[256];  // pattern from config file. Used to generate 'data'
    char const *class_;
};

#endif /* SQUID_ACL_RANDOM_H */

