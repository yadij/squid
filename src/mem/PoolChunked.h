/*
 * Copyright (C) 1996-2022 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef _MEM_POOL_CHUNKED_H_
#define _MEM_POOL_CHUNKED_H_

#include "mem/Allocator.h"
#include "splay.h"

#define MEM_CHUNK_SIZE        4 * 4096  /* 16KB ... 4 * VM_PAGE_SZ */
#define MEM_CHUNK_MAX_SIZE  256 * 1024  /* 2MB */

class MemChunk;

/// \ingroup MemPoolsAPI
class MemPoolChunked : public Mem::Allocator
{
public:
    friend class MemChunk;
    MemPoolChunked(const char *label, size_t obj_size);
    ~MemPoolChunked();
    void convertFreeCacheToChunkFreeCache();
    void createChunk();
    void *get();
    void push(void *obj);

    /* Mem::Allocator API */
    virtual size_t getStats(Mem::PoolStats &);
    virtual void setChunkSize(size_t);
    virtual bool idleTrigger(int) const;
    virtual void clean(time_t);

protected:
    /* Mem::Allocator API */
    virtual void *allocate();
    virtual void deallocate(void *);

public:
    size_t chunk_size;
    int chunk_capacity;
    int chunkCount;
    void *freeCache;
    MemChunk *nextFreeChunk;
    MemChunk *Chunks;
    Splay<MemChunk *> allChunks;
};

/// \ingroup MemPoolsAPI
class MemChunk
{
public:
    MemChunk(MemPoolChunked *pool);
    ~MemChunk();
    void *freeList;
    void *objCache;
    int inuse_count;
    MemChunk *nextFreeChunk;
    MemChunk *next;
    time_t lastref;
    MemPoolChunked *pool;
};

#endif /* _MEM_POOL_CHUNKED_H_ */

