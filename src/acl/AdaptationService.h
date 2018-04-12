/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_SRC_ACL_ADAPTATIONSERVICE_H
#define SQUID_SRC_ACL_ADAPTATIONSERVICE_H

#include "acl/Strategy.h"

namespace Acl {

class AdaptationServiceStrategy : public ACLStrategy<const char *>
{
public:
    virtual int match(Acl::Data<MatchType> * &, ACLFilledChecklist *) override;
};

} // namespace Acl

#endif /* SQUID_SRC_ACL_ADAPTATIONSERVICE_H */

