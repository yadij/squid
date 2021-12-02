/*
 * Copyright (C) 1996-2021 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACL_RANDOM_H
#define SQUID_ACL_RANDOM_H

#include "acl/Acl.h"

class ACLRandom : public ACL
{
    MEMPROXY_CLASS(ACLRandom);

public:
    ACLRandom(char const *);
    ACLRandom(ACLRandom const &);
    virtual ~ACLRandom() {}

    ACLRandom &operator =(ACLRandom const &) = delete;

    /* ACL API */
    virtual ACL *clone()const;
    virtual char const *typeString() const { return class_; }
    virtual void parse();
    virtual int match(ACLChecklist *);
    virtual SBufList dump() const;
    virtual bool empty() const { return data != 0.0; }
    virtual bool valid() const { return !empty(); }

protected:
    double data = 0.0; ///< value to be exceeded before this ACL will match
    char pattern[256]; ///< pattern from config file. Used to generate 'data'
    char const *class_;
};

#endif /* SQUID_ACL_RANDOM_H */

