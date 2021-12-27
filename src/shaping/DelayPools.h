/*
 * Copyright (C) 1996-2021 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef _SQUID__SRC_SHAPING_DELAYPOOLS_H
#define _SQUID__SRC_SHAPING_DELAYPOOLS_H

#include "shaping/forward.h"

#include <vector>

class DelayPool;
class StoreEntry;
class Updateable;

class Updateable
{

public:
    virtual ~Updateable() {}

    virtual void update(int) = 0;
};

class DelayPools
{

public:
    static void Init();
    static void Update(void *);
    static unsigned short pools();
    static void pools(unsigned short);
    static void FreePools();
    static unsigned char *DelayClasses();
    static void registerForUpdates(Updateable *);
    static void deregisterForUpdates(Updateable *);

    static DelayPool *delay_data;

private:
    static void Stats(StoreEntry *);
    static void InitDelayData();
    static void FreeDelayData();
    static void RegisterWithCacheManager();

    static time_t LastUpdate;
    static unsigned short pools_;
    static std::vector<Updateable *> toUpdate;
};

#endif /* _SQUID__SRC_SHAPING_DELAYPOOLS_H */
