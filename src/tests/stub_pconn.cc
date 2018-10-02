/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/*
 * STUB file for the pconn.cc API
 */
#include "squid.h"
#include "comm/Connection.h"

#define STUB_API "pconn.cc"
#include "tests/STUB.h"

#include "pconn.h"
PconnPool::PconnPool(const char *, const CbcPointer<PeerPoolMgr>&) STUB
PconnPool::~PconnPool() STUB
void PconnPool::moduleInit() STUB
void PconnPool::push(const Comm::ConnectionPointer &serverConn, const char *domain) STUB
Comm::ConnectionPointer PconnPool::pop(const Comm::ConnectionPointer &destLink, const char *domain, bool retriable) STUB_RETVAL(Comm::ConnectionPointer())
void PconnPool::count(int uses) STUB
void PconnPool::noteUses(int) STUB
void PconnPool::dumpHist(StoreEntry *e) const STUB
void PconnPool::dumpHash(StoreEntry *e) const STUB
void PconnPool::unlinkList(IdleConnList *list) STUB

