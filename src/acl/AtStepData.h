/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLATSTEPDATA_H
#define SQUID_ACLATSTEPDATA_H

#if USE_OPENSSL

#include "acl/Data.h"
#include "ssl/support.h"

#include <list>

class ACLAtStepData : public Acl::Data<Ssl::BumpStep>
{
    MEMPROXY_CLASS(ACLAtStepData);

public:
    ACLAtStepData();
    ACLAtStepData(ACLAtStepData const &);
    virtual ~ACLAtStepData();
    ACLAtStepData &operator= (ACLAtStepData const &);

    /* Acl::Data<T> API */
    virtual bool match(Ssl::BumpStep) override;
    virtual SBufList dump() const override;
    virtual void parse() override;
    virtual Acl::Data<Ssl::BumpStep> *clone() const override;
    virtual bool empty() const override;

public:
    std::list<Ssl::BumpStep> values;
};

#endif /* USE_OPENSSL */

#endif /* SQUID_ACLSSL_ERRORDATA_H */

