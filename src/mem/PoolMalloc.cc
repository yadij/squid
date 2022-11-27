/*
 * Copyright (C) 1996-2022 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/*
 * AUTHOR: Alex Rousskov, Andres Kroonmaa, Robert Collins, Henrik Nordstrom
 */

#include "squid.h"
#include "mem/PoolMalloc.h"
#include "mem/Stats.h"

#include <cassert>
#include <cstring>

extern time_t squid_curtime;

void *
MemPoolMalloc::allocate()
{
    void *obj = nullptr;
    if (!freelist.empty()) {
        obj = freelist.top();
        freelist.pop();
    }
    if (obj) {
        --(getMeter().idle);
        ++count.saved_allocs;
    } else {
        if (doZero)
            obj = xcalloc(1, objectSize());
        else
            obj = xmalloc(objectSize());
        ++(getMeter().alloc);
    }
    ++(getMeter().inuse);
    return obj;
}

void
MemPoolMalloc::deallocate(void *obj, bool aggressive)
{
    --(getMeter().inuse);
    if (aggressive) {
        xfree(obj);
        --(getMeter().alloc);
    } else {
        if (doZero)
            memset(obj, 0, objectSize());
        ++(getMeter().idle);
        freelist.push(obj);
    }
}

/* TODO extract common logic to MemAllocate */
size_t
MemPoolMalloc::getStats(Mem::PoolStats &stats)
{
    stats.pool = this;
    stats.label = objectType();
    stats.meter = &getMeter();
    stats.obj_size = objectSize();
    stats.chunk_capacity = 0;

    stats.items_alloc += getMeter().alloc.currentLevel();
    stats.items_inuse += getMeter().inuse.currentLevel();
    stats.items_idle += getMeter().idle.currentLevel();

    stats.overhead += sizeof(MemPoolMalloc) + strlen(objectType()) + 1;

    return getMeter().inuse.currentLevel();
}

int
MemPoolMalloc::getInUseCount()
{
    return getMeter().inuse.currentLevel();
}

MemPoolMalloc::MemPoolMalloc(char const *aLabel, size_t aSize) : MemImplementingAllocator(aLabel, aSize)
{
}

MemPoolMalloc::~MemPoolMalloc()
{
    assert(getMeter().inuse.currentLevel() == 0);
    clean(0);
}

bool
MemPoolMalloc::idleTrigger(int shift) const
{
    return freelist.size() >> (shift ? 8 : 0);
}

void
MemPoolMalloc::clean(time_t)
{
    while (!freelist.empty()) {
        void *obj = freelist.top();
        freelist.pop();
        --(getMeter().idle);
        --(getMeter().alloc);
        xfree(obj);
    }
}

