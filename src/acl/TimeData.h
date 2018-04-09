/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLTIMEDATA_H
#define SQUID_ACLTIMEDATA_H

#include "acl/Data.h"

class ACLTimeData : public Acl::Data<time_t>
{
    MEMPROXY_CLASS(ACLTimeData);

public:
    ACLTimeData();
    ACLTimeData(ACLTimeData const &);
    ACLTimeData&operator=(ACLTimeData const &);
    virtual ~ACLTimeData();

    /* Acl::Data<T> API */
    virtual bool match(time_t) override;
    virtual SBufList dump() const override;
    virtual void parse() override;
    virtual Acl::Data<time_t> *clone() const override;
    virtual bool empty() const override;

private:
    int weekbits;
    int start;
    int stop;
    ACLTimeData *next;
};

#endif /* SQUID_ACLTIMEDATA_H */

