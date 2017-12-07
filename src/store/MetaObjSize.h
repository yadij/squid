/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID__STORE_METAOBJSIZE_H
#define SQUID__STORE_METAOBJSIZE_H

#include "store/MetaTlv.h"

class StoreMetaObjSize : public Store::MetaTlv
{
    MEMPROXY_CLASS(StoreMetaObjSize);

public:
    char getType() const {return STORE_META_OBJSIZE;}
};

#endif /* SQUID__STORE_METAOBJSIZE_H */

