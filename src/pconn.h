/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_PCONN_H
#define SQUID_PCONN_H

#include "base/CbcPointer.h"
#include "base/RunnersRegistry.h"
#include "comm/forward.h"
#include "mgr/forward.h"

#include <set>

/**
 \defgroup PConnAPI Persistent Connection API
 \ingroup Component
 *
 \todo CLEANUP: Break multiple classes out of the generic pconn.h header
 */

class PconnPool;
class PeerPoolMgr;

#include "cbdata.h"
#include "hash.h"
/* for IOCB */
#include "comm.h"

/// \ingroup PConnAPI
#define PCONN_HIST_SZ (1<<16)

#include "ip/forward.h"

class StoreEntry;
class IdleConnLimit;

/* for hash_table */
#include "hash.h"

/** \ingroup PConnAPI
 * Manages idle persistent connections to a caller-defined set of
 * servers (e.g., all HTTP servers). Uses a collection of IdleConnLists
 * internally to list the individual open connections to each server.
 * Controls lists existence and limits the total number of
 * idle connections across the collection.
 */
class PconnPool
{

public:
    PconnPool(const char *aDescription, const CbcPointer<PeerPoolMgr> &aMgr);
    ~PconnPool();

    void moduleInit();
    void push(const Comm::ConnectionPointer &serverConn, const char *domain);

    /**
     * Returns either a pointer to a popped connection to dest or nil.
     * Closes the connection before returning its pointer unless keepOpen.
     *
     * A caller with a non-retriable transaction should set keepOpen to false
     * and call pop() anyway, even though the caller does not want a pconn.
     * This forces us to close an available persistent connection, avoiding
     * creating a growing number of open connections when many transactions
     * create (and push) persistent connections but are not retriable and,
     * hence, do not need to pop a connection.
     */
    Comm::ConnectionPointer pop(const Comm::ConnectionPointer &dest, const char *domain, bool keepOpen);
    void count(int uses);
    void dumpHist(StoreEntry *e) const;
    void dumpHash(StoreEntry *e) const;
    void unlinkList(IdleConnList *list);
    void noteUses(int uses);
    /// closes any n connections, regardless of their destination
    void closeN(int n);
    int count() const { return theCount; }
    void noteConnectionAdded() { ++theCount; }
    void noteConnectionRemoved() { assert(theCount > 0); --theCount; }

    // sends an async message to the pool manager, if any
    void notifyManager(const char *reason);

private:

    static const char *key(const Comm::ConnectionPointer &destLink, const char *domain);

    int hist[PCONN_HIST_SZ];
    hash_table *table;
    const char *descr;
    CbcPointer<PeerPoolMgr> mgr; ///< optional pool manager (for notifications)
    int theCount; ///< the number of pooled connections
};

class StoreEntry;
class PconnPool;

/** \ingroup PConnAPI
 * The global registry of persistent connection pools.
 */
class PconnModule
{

public:
    /** the module is a singleton until we have instance based cachemanager
     * management
     */
    static PconnModule * GetInstance();
    /** A thunk to the still C like CacheManager callback api. */
    static void DumpWrapper(StoreEntry *e);

    PconnModule();
    void registerWithCacheManager(void);

    void add(PconnPool *);
    void remove(PconnPool *); ///< unregister and forget about this pool object

    OBJH dump;

private:
    typedef std::set<PconnPool*> Pools; ///< unordered PconnPool collection
    Pools pools; ///< all live pools

    static PconnModule * instance;
};

#endif /* SQUID_PCONN_H */

