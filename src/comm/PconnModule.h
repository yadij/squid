/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef _SQUID_SRC_COMM_PCONNMODULE_H
#define _SQUID_SRC_COMM_PCONNMODULE_H

#include "comm/forward.h"
// for OBJH
#include "mgr/forward.h"

class StoreEntry;

namespace Comm
{

/**
 * The global registry of persistent connection pools.
 * This simple class exists only for the cache manager
 */
class PconnModule
{
public:
    PconnModule();

    /** the module is a singleton until we have instance based cachemanager
     * management
     */
    static PconnModule * GetInstance();
    /** A thunk to the still C like CacheManager callback api. */
    static void DumpWrapper(StoreEntry *);

    /// register a pool for cache manager report generation
    void add(PconnPool *);

    /// unregister and forget about this pool object
    void remove(PconnPool *);

    OBJH dump;

private:
    std::set<PconnPool*> pools; ///< collection of registered pools
};

} // namespace Comm

#endif /* _SQUID_SRC_COMM_PCONNMODULE_H */

