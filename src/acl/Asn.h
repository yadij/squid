/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLASN_H
#define SQUID_ACLASN_H

#include "acl/Data.h"
#include "base/CbDataList.h"
#include "ip/Address.h"

int asnMatchIp(CbDataList<int> *, Ip::Address &);

/// \ingroup ACLAPI
void asnInit(void);

/// \ingroup ACLAPI
void asnFreeMemory(void);

/// \ingroup ACLAPI
class ACLASN : public ACLData<Ip::Address>
{
    MEMPROXY_CLASS(ACLASN);

public:
    ACLASN() : data(nullptr) {}
    virtual ~ACLASN();

    /* ACLData<T> API */
    virtual bool match(Ip::Address) override;
    virtual SBufList dump() const override;
    virtual void parse() override;
    virtual ACLData<Ip::Address> *clone() const override;
    virtual void prepareForUse() override;
    virtual bool empty() const override;

private:
    CbDataList<int> *data;
};

#endif /* SQUID_ACLASN_H */

