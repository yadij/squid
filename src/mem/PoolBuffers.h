/*
 * Copyright (C) 1996-2022 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID__SRC_MEM_POOLBUFFERS_H
#define SQUID__SRC_MEM_POOLBUFFERS_H

#include "mem/Pool.h"

#include <list>

namespace Mem
{

/// Memory Pools allocator for variable-length buffers
class PoolBuffers : public MemImplementingAllocator
{
public:
    PoolBuffers(char const *label, size_t defaultSize);
    ~PoolBuffers();

    static PoolBuffers &Instance();

    /// allocate a buffer of at least the given size
    void *allocate(size_t sz) { return allocate(&sz); }

    /// allocate a buffer of at least the given size
    /// and update 'sz' to the actual size allocated
    void *allocate(size_t *sz);

    /// deallocate a buffer we allocated
    void deallocate(void *, size_t);

    /* Mem::Allocator API */
    virtual int getStats(MemPoolStats *, int);
    virtual int getInUseCount();
    virtual void setChunkSize(size_t);

    /* MemImplementingAllocator API */
    virtual bool idleTrigger(int) const { return false; }
    virtual void clean(time_t);

protected:
    /* MemImplementingAllocator API */
    virtual void *allocate();
    virtual void deallocate(void *, bool);
};

} // namespace Mem

#endif /* SQUID__SRC_MEM_POOLBUFFERS_H */
