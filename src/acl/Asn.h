/*
 * Copyright (C) 1996-2021 The Squid Software Foundation and contributors
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
    virtual ~ACLASN();

    /* ACLData API */
    virtual bool match(Ip::Address) override;
    virtual SBufList dump() const override;
    virtual void parse() override;
    virtual bool empty() const override { return data == nullptr; }
    virtual ACLData<Ip::Address> *clone() const override;
    virtual void prepareForUse() override;

private:
    CbDataList<int> *data = nullptr;
};

#endif /* SQUID_ACLASN_H */

