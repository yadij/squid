/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLARP_H
#define SQUID_ACLARP_H

#include "acl/MatchNode.h"

#include <set>

namespace Eui
{
class Eui48;
};

class ACLARP : public Acl::MatchNode
{
    MEMPROXY_CLASS(ACLARP);

public:
    ACLARP(char const *);
    ACLARP(ACLARP const &);
    ~ACLARP() {}
    ACLARP&operator=(ACLARP const &);

    /* Acl::MatchNode API */
    virtual void parse() override;
    virtual char const *typeString() const override;
    virtual SBufList dump() const override;
    virtual bool empty() const override;
    virtual int match(ACLChecklist *) override;

protected:
    char const *class_;
    typedef std::set<Eui::Eui48> AclArpData_t;
    AclArpData_t aclArpData;
};

#endif /* SQUID_ACLARP_H */

