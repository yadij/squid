/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLINTRANGE_H
#define SQUID_ACLINTRANGE_H

#include "acl/Data.h"
#include "base/Range.h"

#include <list>

class ACLIntRange : public Acl::Data<int>
{

public:
    ACLIntRange() {}
    virtual ~ACLIntRange();

    /* Acl::Data<T> API */
    virtual bool match(int) override;
    virtual SBufList dump() const override;
    virtual void parse() override;
    virtual Acl::Data<int> *clone() const override;
    virtual bool empty() const override;

private:
    typedef Range<int> RangeType;
    std::list<RangeType> ranges;
};

#endif /* SQUID_ACLINTRANGE_H */

