/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID__STORE_METASTDLFS_H
#define SQUID__STORE_METASTDLFS_H

#include "store/MetaTlv.h"

class StoreMetaSTDLFS : public Store::MetaTlv
{
    MEMPROXY_CLASS(StoreMetaSTDLFS);

public:
    char getType() const {return STORE_META_STD_LFS;}

    bool validLength(int) const;
    //    bool checkConsistency(StoreEntry *) const;
};

#endif /* SQUID__STORE_METASTDLFS_H */

