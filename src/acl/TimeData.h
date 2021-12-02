/*
 * Copyright (C) 1996-2021 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLTIMEDATA_H
#define SQUID_ACLTIMEDATA_H

#include "acl/Acl.h"
#include "acl/Data.h"

class ACLTimeData : public ACLData<time_t>
{
    MEMPROXY_CLASS(ACLTimeData);

public:
    virtual ~ACLTimeData();

    /* ACLData API */
    bool match(time_t);
    virtual SBufList dump() const;
    void parse();
    bool empty() const { return false; }
    virtual ACLData<time_t> *clone() const;

private:
    int weekbits = 0;
    int start = 0;
    int stop = 0;
    ACLTimeData *next = nullptr;
};

#endif /* SQUID_ACLTIMEDATA_H */

