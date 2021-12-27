/*
 * Copyright (C) 1996-2021 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "base/RunnersRegistry.h"
#include "CommonPool.h"
#include "DelayPool.h"
#include "mgr/Registration.h"
#include "shaping/DelayPools.h"
#include "Store.h"

DelayPool *DelayPools::delay_data = nullptr;
time_t DelayPools::LastUpdate = 0;
unsigned short DelayPools::pools_ = 0;

namespace Shaping
{

class DelayPoolsRr : public RegisteredRunner
{
public:
    /* RegisteredRunner API */
    virtual void finishShutdown() { DelayPools::FreePools(); }
};

RunnerRegistrationEntry(DelayPoolsRr);

} // namespace Shaping

void
DelayPools::RegisterWithCacheManager(void)
{
    Mgr::RegisterAction("delay", "Delay Pool Levels", Stats, 0, 1);
}

void
DelayPools::Init()
{
    LastUpdate = getCurrentTime();
    RegisterWithCacheManager();
}

void
DelayPools::InitDelayData()
{
    if (!pools())
        return;

    DelayPools::delay_data = new DelayPool[pools()];

    eventAdd("DelayPools::Update", DelayPools::Update, NULL, 1.0, 1);
}

void
DelayPools::FreeDelayData()
{
    eventDelete(DelayPools::Update, NULL);
    delete[] DelayPools::delay_data;
    pools_ = 0;
}

void
DelayPools::Update(void *)
{
    if (!pools())
        return;

    eventAdd("DelayPools::Update", Update, NULL, 1.0, 1);

    int incr = squid_curtime - LastUpdate;

    if (incr < 1)
        return;

    LastUpdate = squid_curtime;

    std::vector<Updateable *>::iterator pos = toUpdate.begin();

    while (pos != toUpdate.end()) {
        (*pos)->update(incr);
        ++pos;
    }
}

void
DelayPools::registerForUpdates(Updateable *anObject)
{
    /* Assume no doubles */
    toUpdate.push_back(anObject);
}

void
DelayPools::deregisterForUpdates(Updateable *anObject)
{
    std::vector<Updateable *>::iterator pos = toUpdate.begin();

    while (pos != toUpdate.end() && *pos != anObject) {
        ++pos;
    }

    if (pos != toUpdate.end()) {
        /* move all objects down one */
        std::vector<Updateable *>::iterator temp = pos;
        ++pos;

        while (pos != toUpdate.end()) {
            *temp = *pos;
            ++temp;
            ++pos;
        }

        toUpdate.pop_back();
    }
}

std::vector<Updateable *> DelayPools::toUpdate;

void
DelayPools::Stats(StoreEntry *sentry)
{
    storeAppendPrintf(sentry, "Delay pools configured: %d\n\n", DelayPools::pools());

    for (unsigned short i = 0; i < DelayPools::pools(); ++i) {
        if (DelayPools::delay_data[i].theComposite().getRaw()) {
            storeAppendPrintf(sentry, "Pool: %d\n\tClass: %s\n\n", i + 1, DelayPools::delay_data[i].pool->theClassTypeLabel());
            DelayPools::delay_data[i].theComposite()->stats (sentry);
        } else {
            storeAppendPrintf(sentry, "\tMisconfigured pool.\n\n");
        }
    }
}

void
DelayPools::FreePools()
{
    if (!DelayPools::pools())
        return;

    FreeDelayData();
}

unsigned short
DelayPools::pools()
{
    return pools_;
}

void
DelayPools::pools(unsigned short newPools)
{
    if (pools()) {
        debugs(3, DBG_CRITICAL, "parse_delay_pool_count: multiple delay_pools lines, aborting all previous delay_pools config");
        FreePools();
    }

    pools_ = newPools;

    if (pools())
        InitDelayData();
}
