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
#include "mem/Pool.h"
#include "mem/PoolMalloc.h"

#include <cassert>
#include <cstring>

extern time_t squid_curtime;

void *
Mem::PoolMalloc::allocate()
{
    void *obj = nullptr;
    if (!freelist.empty()) {
        obj = freelist.top();
        freelist.pop();
    }
    if (obj) {
        --meter.idle;
        ++saved_calls;
    } else {
        if (doZero)
            obj = xcalloc(1, obj_size);
        else
            obj = xmalloc(obj_size);
        ++meter.alloc;
    }
    ++meter.inuse;
    return obj;
}

void
Mem::PoolMalloc::deallocate(void *obj, bool aggressive)
{
    --meter.inuse;
    if (aggressive) {
        xfree(obj);
        --meter.alloc;
    } else {
        if (doZero)
            memset(obj, 0, obj_size);
        ++meter.idle;
        freelist.push(obj);
    }
}

/* TODO extract common logic to MemAllocate */
int
Mem::PoolMalloc::getStats(PoolStats * stats)
{
    stats->pool = this;
    stats->label = objectType();
    stats->meter = &meter;
    stats->obj_size = obj_size;
    stats->chunk_capacity = 0;

    stats->chunks.alloc += 0;
    stats->chunks.inuse += 0;
    stats->chunks.partial += 0;
    stats->chunks.free += 0;

    stats->items.alloc += meter.alloc.currentLevel();
    stats->items.inuse += meter.inuse.currentLevel();
    stats->items.idle += meter.idle.currentLevel();

    stats->overhead += sizeof(PoolMalloc) + strlen(objectType()) + 1;

    return meter.inuse.currentLevel();
}

int
Mem::PoolMalloc::getInUseCount()
{
    return meter.inuse.currentLevel();
}

Mem::PoolMalloc::PoolMalloc(char const *aLabel, size_t aSize) :
    Mem::AllocatorMetrics(aLabel, aSize)
{
}

Mem::PoolMalloc::~PoolMalloc()
{
    assert(meter.inuse.currentLevel() == 0);
    clean(0);
}

bool
Mem::PoolMalloc::idleTrigger(int shift) const
{
    return freelist.size() >> (shift ? 8 : 0);
}

void
Mem::PoolMalloc::clean(time_t)
{
    while (!freelist.empty()) {
        void *obj = freelist.top();
        freelist.pop();
        --meter.idle;
        --meter.alloc;
        xfree(obj);
    }
}

