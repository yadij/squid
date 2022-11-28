/*
 * Copyright (C) 1996-2022 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "debug/Stream.h"
#include "mem/PoolBuffers.h"

extern time_t squid_curtime;

namespace Mem
{
static std::list<MemImplementingAllocator *> TheAllocators;
}

Mem::PoolBuffers::PoolBuffers(char const *aLabel, size_t defaultSize) :
    MemImplementingAllocator(aLabel, MemAllocator::RoundedSize(512) /* largest 'Strings' pool size */)
{
    // start with odd-sized pools for Strings
    static const struct {
        const char *name;
        size_t obj_size;
    } PoolAttrs[] = {
        {"Short Strings", MemAllocator::RoundedSize(36)}, /* to fit rfc1123 and similar */
        {"Medium Strings", MemAllocator::RoundedSize(128)}, /* to fit most urls */
        {"Long Strings", MemAllocator::RoundedSize(512)}
    };

    if (TheAllocators.empty()) {
        for (const auto &meta : PoolAttrs) {
            auto pool = MemPools::GetInstance().create(meta.name, meta.obj_size);
            pool->zeroBlocks(false);

            if (pool->objectSize() != meta.obj_size)
                debugs(13, DBG_IMPORTANT, "WARNING: " << meta.name <<
                       " is " << pool->objectSize() <<
                       " bytes instead of requested " <<
                       meta.obj_size << " bytes");
            TheAllocators.emplace_back(pool);
        }
    }

    // auto-initialize 1KB+ buffers only as needed
    if (defaultSize > objectSize())
        setChunkSize(defaultSize);
}

Mem::PoolBuffers::~PoolBuffers()
{
    assert(meter.inuse.currentLevel() == 0);
    clean(0);
}

Mem::PoolBuffers &
Mem::PoolBuffers::Instance()
{
    static PoolBuffers instance("Buffers", 0);
    return instance;
}

void
Mem::PoolBuffers::setChunkSize(size_t wantBufSz)
{
    // limit buffers 4MB (one 2^n increment larger than SBuf limit)
    assert(wantBufSz <= (4<<20));

    size_t largest = objectSize();
    while (largest < wantBufSz) {
        size_t newSz = (largest << 1);
        auto name = static_cast<char *>(xmalloc(128)); // XXX: leaks on shutdown with pool destruct
        (void)snprintf(name, 128, "%" PRIuSIZE "KB X-%s", (largest>>10), objectType());
        auto pool = MemPools::GetInstance().create(name, newSz);
        pool->zeroBlocks(false);
        TheAllocators.emplace_back(pool);
        largest = newSz;
    }
    obj_size = largest;
}

int
Mem::PoolBuffers::getInUseCount()
{
    int total = meter.inuse.currentLevel();
    for (const auto pool: TheAllocators)
        total = pool->getMeter().inuse.currentLevel();
    return total;
}

void
Mem::PoolBuffers::clean(time_t t)
{
    for (const auto pool: TheAllocators)
        pool->clean(t);
}

void *
Mem::PoolBuffers::allocate()
{
    size_t wantSz = objectSize();
    return allocate(&wantSz);
}

void *
Mem::PoolBuffers::allocate(size_t *sz)
{
    if (*sz > objectSize())
        setChunkSize(*sz); // grow available pools

    for (const auto pool: TheAllocators) {
        if (pool->objectSize() >= *sz) {
            *sz = pool->objectSize();
            return pool->alloc();
        }
    }

    // UNREACHABLE
    assert(*sz <= objectSize());
    // worst-case produce our current largest buffer size
    return allocate();
}

void
Mem::PoolBuffers::deallocate(void *obj, size_t sz)
{
    for (const auto pool: TheAllocators) {
        if (pool->objectSize() >= sz) {
            pool->freeOne(obj);
            return;
        }
    }
    // UNREACHABLE? will leak any memory we did not allocate()
}

void
Mem::PoolBuffers::deallocate(void *obj, bool)
{
    char *buf = static_cast<char *>(obj);
    deallocate(buf, sizeof(buf));
}

int
Mem::PoolBuffers::getStats(MemPoolStats * stats, int accumulate)
{
    if (!accumulate)    /* need skip memset for GlobalStats accumulation */
        memset(stats, 0, sizeof(MemPoolStats));

    stats->pool = this;
    stats->label = objectType();
    stats->meter = &meter;
    stats->obj_size = obj_size;

    stats->overhead += sizeof(Mem::PoolBuffers) + strlen(objectType()) + 1;
    return 0;
}
