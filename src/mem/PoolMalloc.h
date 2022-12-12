/*
 * Copyright (C) 1996-2022 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef _MEM_POOL_MALLOC_H_
#define _MEM_POOL_MALLOC_H_

/**
 \defgroup MemPoolsAPI  Memory Management (Memory Pool Allocator)
 \ingroup Components
 *
 *\par
 *  MemPools are a pooled memory allocator running on top of malloc(). It's
 *  purpose is to reduce memory fragmentation and provide detailed statistics
 *  on memory consumption.
 *
 \par
 *  Preferably all memory allocations in Squid should be done using MemPools
 *  or one of the types built on top of it (i.e. cbdata).
 *
 \note Usually it is better to use cbdata types as these gives you additional
 *     safeguards in references and typechecking. However, for high usage pools where
 *     the cbdata functionality of cbdata is not required directly using a MemPool
 *     might be the way to go.
 */

#include "mem/Allocator.h"

#include <stack>

/// \ingroup MemPoolsAPI
class MemPoolMalloc : public Mem::Allocator
{
public:
    MemPoolMalloc(char const *label, size_t aSize);
    ~MemPoolMalloc();

    /* Mem::Allocator API */
    virtual size_t getStats(Mem::PoolStats &);
    virtual int getInUseCount();
    virtual bool idleTrigger(int) const;
    virtual void clean(time_t);

protected:
    /* Mem::Allocator API */
    virtual void *allocate();
    virtual void deallocate(void *);

private:
    std::stack<void *> freelist;
};

#endif /* _MEM_POOL_MALLOC_H_ */

