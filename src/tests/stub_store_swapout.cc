/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "store/MetaTlv.h"

#define STUB_API "store_swapout.cc"
#include "tests/STUB.h"

#include <iostream>

/* XXX: wrong stub file... */
void storeUnlink(StoreEntry * e) STUB

char *storeSwapMetaPack(Store::MetaTlv *t, int *) STUB_RETVAL(nullptr)
Store::MetaTlv *storeSwapMetaBuild(StoreEntry *) STUB_RETVAL(nullptr)
void storeSwapTLVFree(Store::MetaTlv *) STUB

