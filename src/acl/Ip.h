/*
 * Copyright (C) 1996-2021 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLIP_H
#define SQUID_ACLIP_H

#include "acl/Acl.h"
#include "acl/Data.h"
#include "ip/Address.h"
#include "splay.h"

class acl_ip_data
{
    MEMPROXY_CLASS(acl_ip_data);

public:
    acl_ip_data() = default;
    acl_ip_data(Ip::Address const &, Ip::Address const &, Ip::Address const &, acl_ip_data *);

    static acl_ip_data *FactoryParse(char const *);
    static int NetworkCompare(acl_ip_data * const & a, acl_ip_data * const &b);

    void toStr(char *buf, int len) const;
    SBuf toSBuf() const;

    Ip::Address addr1;
    Ip::Address addr2;
    Ip::Address mask; // TODO: should use a CIDR range

    acl_ip_data *next = nullptr; ///< used for parsing, not for storing

private:
    static bool DecodeMask(const char *asc, Ip::Address &mask, int string_format_type);
};

class ACLIP : public ACL
{
public:
    typedef Splay<acl_ip_data *> IPSplay;

    virtual ~ACLIP();

    /* ACL API */
    virtual void parse() override;
    virtual SBufList dump() const override;
    virtual bool empty() const override;
    virtual int match(ACLChecklist *) override = 0;

protected:
    int match(const Ip::Address &);

    IPSplay *data = nullptr;
};

#endif /* SQUID_ACLIP_H */

