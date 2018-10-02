/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef _SQUID_SRC_COMM_PCONNMODULE_H
#define _SQUID_SRC_COMM_PCONNMODULE_H

// for OBJH
#include "mgr/forward.h"

class StoreEntry;
class PconnPool;

/**
 * The global registry of persistent connection pools.
 * This simple class exists only for the cache manager
 */
class PconnModule
{

public:
    /** the module is a singleton until we have instance based cachemanager
     * management
     */
    static PconnModule * GetInstance();
    /** A thunk to the still C like CacheManager callback api. */
    static void DumpWrapper(StoreEntry *);

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

#endif /* _SQUID_SRC_COMM_PCONNMODULE_H */

