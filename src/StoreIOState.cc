/*
 * Copyright (C) 1996-2017 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 20    Swap Dir base object */

#include "squid.h"
#include "Debug.h"
#include "Store.h"
#include "StoreIOState.h"

void *
StoreIOState::operator new (size_t)
{
    assert(0);
    return (void *)1;
}

void
StoreIOState::operator delete (void *)
{
    assert(0);
}

StoreIOState::StoreIOState(StoreIOState::STIOCB *cbIo, void *data) :
    callback(cbIo),
    callback_data(cbdataReference(data))
{
}

StoreIOState::~StoreIOState()
{
    debugs(20,3, "StoreIOState::~StoreIOState: " << this);

    if (read.callback_data)
        cbdataReferenceDone(read.callback_data);

    if (callback_data)
        cbdataReferenceDone(callback_data);
}

bool StoreIOState::touchingStoreEntry() const
{
    return e && e->swap_filen == swap_filen;
}

