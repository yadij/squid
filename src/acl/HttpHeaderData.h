/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLHTTPHEADERDATA_H
#define SQUID_ACLHTTPHEADERDATA_H

#include "acl/Data.h"
#include "HttpHeader.h"
#include "sbuf/SBuf.h"
#include "SquidString.h"

class ACLHTTPHeaderData : public Acl::Data<HttpHeader*>
{
    MEMPROXY_CLASS(ACLHTTPHeaderData);

public:
    ACLHTTPHeaderData();
    virtual ~ACLHTTPHeaderData();

    /* Acl::Data<T> API */
    virtual bool match(HttpHeader *) override;
    virtual SBufList dump() const override;
    virtual void parse() override;
    virtual Acl::Data<HttpHeader*> *clone() const override;
    virtual bool empty() const override;

private:
    Http::HdrType hdrId;            /**< set if header is known */
    SBuf hdrName;                   /**< always set */
    Acl::Data<char const *> * regex_rule;
};

#endif /* SQUID_ACLHTTPHEADERDATA_H */

