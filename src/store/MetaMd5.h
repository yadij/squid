/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID__STORE_METAMD5_H
#define SQUID__STORE_METAMD5_H

#include "store/MetaTlv.h"

class StoreMetaMD5 : public Store::MetaTlv
{
    MEMPROXY_CLASS(StoreMetaMD5);

public:
    char getType() const {return STORE_META_KEY_MD5;}

    bool validLength(int) const;
    bool checkConsistency(StoreEntry *) const;

private:
    static int md5_mismatches;
};

#endif /* SQUID__STORE_METAMD5_H */

