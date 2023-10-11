/*
 * Copyright (C) 1996-2023 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "Store.h"

#define STUB_API "store_client.cc"
#include "tests/STUB.h"

// XXX: stubs that should be elsewhere
#include "store_log.h"
void storeLog(int, const StoreEntry *) STUB_NOP
void storeLogOpen(void) STUB

// XXX: misplaced definition, should not be in store_client.cc
void StoreEntry::invokeHandlers() STUB_NOP

#include "StoreClient.h"
/*protected:
bool StoreClient::startCollapsingOn(const StoreEntry &, const bool) const STUB_RETVAL(false)
bool StoreClient::onCollapsingPath() const STUB_RETVAL(false)
*/
CBDATA_CLASS_INIT(store_client);
store_client::store_client(StoreEntry *) STUB
store_client::~store_client() STUB
int store_client::getType() const STUB_RETVAL(0)
void store_client::noteSwapInDone(bool) STUB
void store_client::doCopy(StoreEntry *) STUB
void store_client::readHeader(const char *, ssize_t) STUB
void store_client::readBody(const char *, ssize_t) STUB
void store_client::copy(StoreEntry *, StoreIOBuffer, STCB *, void *) STUB
void store_client::dumpStats(MemBuf *, int) const STUB
#if USE_DELAY_POOLS
int store_client::bytesWanted() const STUB_RETVAL(0)
void store_client::setDelayId(DelayId) STUB
#endif
store_client::Callback::Callback(STCB *, void *) STUB
bool store_client::Callback::pending() const STUB
void storeClientCopy(store_client *, StoreEntry *, StoreIOBuffer, STCB *, void *) STUB
store_client* storeClientListAdd(StoreEntry *, void *) STUB_RETVAL(nullptr)
int storeUnregister(store_client *, StoreEntry *, void *) STUB_RETVAL(0)
int storePendingNClients(const StoreEntry *) STUB_RETVAL_NOP(0)
int storeClientIsThisAClient(store_client *, void *) STUB_RETVAL(0)

