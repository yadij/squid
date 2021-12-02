/*
 * Copyright (C) 1996-2021 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLSQUIDERRORDATA_H
#define SQUID_ACLSQUIDERRORDATA_H

#include "acl/Data.h"
#include "base/CbDataList.h"
#include "error/forward.h"

/// \ingroup ACLAPI
class ACLSquidErrorData : public ACLData<err_type>
{

public:
    virtual ~ACLSquidErrorData() {}

    /* ACLData API */
    virtual bool match(err_type);
    virtual SBufList dump() const;
    virtual void parse();
    virtual bool empty() const { return errors.empty(); }
    virtual ACLData<err_type> *clone() const;

private:
    CbDataListContainer <err_type> errors;
};

#endif //SQUID_ACLSQUIDERRORDATA_H

