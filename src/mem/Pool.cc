/*
 * Copyright (C) 1996-2022 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/*
 * AUTHOR: Alex Rousskov, Andres Kroonmaa, Robert Collins
 */

#include "squid.h"
#include "mem/PoolChunked.h"
#include "mem/PoolMalloc.h"
#include "mem/Stats.h"

#include <cassert>
#include <cstring>

#define FLUSH_LIMIT 1000    /* Flush memPool counters to memMeters after flush limit calls */

extern time_t squid_curtime;

Mem::PoolMeter TheMeter;

MemPools &
MemPools::GetInstance()
{
    // We must initialize on first use (which may happen during static
    // initialization) and preserve until the last user is gone (which
    // may happen long after main() exit). We currently preserve forever.
    static MemPools *Instance = new MemPools;
    return *Instance;
}

/* Change the default value of defaultIsChunked to override
 * all pools - including those used before main() starts where
 * MemPools::GetInstance().setDefaultPoolChunking() can be called.
 */
MemPools::MemPools()
{
    if (char *cfg = getenv("MEMPOOLS"))
        defaultIsChunked = atoi(cfg);
}

MemImplementingAllocator *
MemPools::create(const char *label, size_t obj_size)
{
    // TODO Use ref-counted Pointer for pool lifecycle management
    // that is complicated by all the global static pool pointers.
    // For now leak these Allocator descendants on shutdown.

    MemImplementingAllocator *newPool;
    if (defaultIsChunked)
        newPool = new MemPoolChunked(label, obj_size);
    else
        newPool = new MemPoolMalloc(label, obj_size);
    pools.push_back(newPool);
    return pools.back();
}

void
MemPools::setDefaultPoolChunking(bool const &aBool)
{
    defaultIsChunked = aBool;
}

/*
 * Updates all pool counters, and recreates TheMeter totals from all pools
 */
void
MemPools::flushMeters()
{
    // Does reset of the historic gb_* counters in TheMeter.
    // This is okay as they get regenerated from pool historic counters.
    TheMeter.flush();

    for (const auto pool: pools) {
        // ensure the pool's meter contains latest high-performance counter values
        pool->flushCounters();

        // Accumulate current volumes (in bytes) across all pools.
        TheMeter.alloc += pool->getMeter().alloc.currentLevel() * pool->objectSize();
        TheMeter.inuse += pool->getMeter().inuse.currentLevel() * pool->objectSize();
        TheMeter.idle += pool->getMeter().idle.currentLevel() * pool->objectSize();
        // Do not accumulate peak details as timing for those vary between pools.

        // regenerate gb_* values from original pool stats
        TheMeter.gb_allocated.count += pool->getMeter().gb_allocated.count;
        TheMeter.gb_saved.count += pool->getMeter().gb_saved.count;
        TheMeter.gb_freed.count += pool->getMeter().gb_freed.count;
        TheMeter.gb_allocated.bytes += pool->getMeter().gb_allocated.bytes;
        TheMeter.gb_saved.bytes += pool->getMeter().gb_saved.bytes;
        TheMeter.gb_freed.bytes += pool->getMeter().gb_freed.bytes;
    }
}

void *
MemImplementingAllocator::alloc()
{
    if (++count.allocs == FLUSH_LIMIT)
        flushCounters();

    return allocate();
}

void
MemImplementingAllocator::freeOne(void *obj)
{
    assert(obj != nullptr);
    (void) VALGRIND_CHECK_MEM_IS_ADDRESSABLE(obj, objectSize());
    deallocate(obj, MemPools::GetInstance().idleLimit() == 0);
    ++count.freed;
}

/*
 * Returns all cached frees to their home chunks
 * If chunks unreferenced age is over, destroys Idle chunk
 * Flushes meters for a pool
 * If pool is not specified, iterates through all pools.
 * When used for all pools, if new_idle_limit is above -1, new
 * idle memory limit is set before Cleanup. This allows to shrink
 * memPool memory usage to specified minimum.
 */
void
MemPools::clean(time_t maxage)
{
    flushMeters();
    if (idleLimit() < 0) // no limit to enforce
        return;

    int shift = 1;
    if (TheMeter.idle.currentLevel() > idleLimit())
        maxage = shift = 0;

    for (const auto pool: pools) {
        if (pool->idleTrigger(shift))
            pool->clean(maxage);
    }
}

MemImplementingAllocator::MemImplementingAllocator(char const * const aLabel, const size_t aSize):
    Mem::Allocator(aLabel, aSize)
{
    assert(aLabel != nullptr && aSize);
}
