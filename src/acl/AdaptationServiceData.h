/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_SRC_ACL_ADAPTATIONSERVICEDATA_H
#define SQUID_SRC_ACL_ADAPTATIONSERVICEDATA_H

#include "acl/StringData.h"

namespace Acl {

class AdaptationServiceData : public ACLStringData
{
public:
    AdaptationServiceData() : ACLStringData() {}
    AdaptationServiceData(AdaptationServiceData const &old) : ACLStringData(old) {}
    AdaptationServiceData &operator= (AdaptationServiceData const &) = delete;

    /* ACLStringData API */
    virtual void parse() override;
    virtual ACLData<char const *> *clone() const override;
};

} // namespace Acl

#endif /* SQUID_SRC_ACL_ADAPTATIONSERVICEDATA_H */

