/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLEUI64_H
#define SQUID_ACLEUI64_H

#include "acl/MatchNode.h"

#include <set>

namespace Eui
{
class Eui64;
};

class ACLEui64 : public Acl::MatchNode
{
    MEMPROXY_CLASS(ACLEui64);

public:
    ACLEui64(char const *);
    ACLEui64(ACLEui64 const &);
    ~ACLEui64() {}
    ACLEui64&operator=(ACLEui64 const &);

    /* Acl::MatchNode API */
    virtual void parse() override;
    virtual char const *typeString() const override;
    virtual SBufList dump() const override;
    virtual bool empty() const override;
    virtual int match(ACLChecklist *) override;

protected:
    typedef std::set<Eui::Eui64> Eui64Data_t;
    Eui64Data_t eui64Data;
    char const *class_;
};

#endif /* SQUID_ACLEUI64_H */

