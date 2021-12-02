/*
 * Copyright (C) 1996-2021 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLUSERDATA_H
#define SQUID_ACLUSERDATA_H

#include "acl/Acl.h"
#include "acl/Data.h"
#include "sbuf/SBuf.h"

#include <set>

class ACLUserData : public ACLData<char const *>
{
    MEMPROXY_CLASS(ACLUserData);

public:
    ACLUserData();
    virtual ~ACLUserData() {}

    /* ACLData API */
    virtual bool match(char const *) override;
    virtual SBufList dump() const override;
    virtual void parse() override;
    virtual const Acl::ParameterFlags &supportedFlags() const override;
    virtual bool empty() const override;
    virtual ACLData<char const *> *clone() const override;

private:
    typedef std::set<SBuf,bool(*)(const SBuf&, const SBuf&)> UserDataNames_t;
    UserDataNames_t userDataNames;

    struct {
        bool case_insensitive = false;
        bool required = false;
    } flags;

};

#endif /* SQUID_ACLUSERDATA_H */

