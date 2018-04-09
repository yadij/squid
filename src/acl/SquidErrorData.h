/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLSQUIDERRORDATA_H
#define SQUID_ACLSQUIDERRORDATA_H

#include "acl/Data.h"
#include "base/CbDataList.h"
#include "err_type.h"

/// \ingroup ACLAPI
class ACLSquidErrorData : public Acl::Data<err_type>
{

public:
    ACLSquidErrorData(): Acl::Data<err_type>() {}
    virtual ~ACLSquidErrorData() {}

    /* Acl::Data<T> API */
    virtual bool match(err_type) override;
    virtual SBufList dump() const override;
    virtual void parse() override;
    virtual Acl::Data<err_type> *clone() const override;
    virtual bool empty() const override;

private:
    CbDataListContainer <err_type> errors;
};

#endif //SQUID_ACLSQUIDERRORDATA_H

