/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"

#define STUB_API "store/libstore.la"
#include "tests/STUB.h"

// libstoremeta.la sub-library linked to store/libstore.la
#include "store/MetaTlv.h"
bool Store::MetaTlv::validType(char) STUB_RETVAL(false)
bool Store::MetaTlv::validLength(int) const STUB_RETVAL(false)
Store::MetaTlv * Store::MetaTlv::Factory(char, size_t, void const *) STUB_RETVAL(nullptr)
void Store::MetaTlv::FreeList(Store::MetaTlv **) STUB
Store::MetaTlv ** Store::MetaTlv::Add(Store::MetaTlv **, Store::MetaTlv *) STUB_RETVAL(nullptr)
bool Store::MetaTlv::checkConsistency(StoreEntry *) const STUB_RETVAL(false)

