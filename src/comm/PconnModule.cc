/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 48    Persistent Connections */

#include "squid.h"
#include "comm/PconnModule.h"
#include "mgr/Registration.h"
#include "pconn.h"
// for StoreEntry
#include "Store.h"

//TODO: re-attach to MemPools. WAS: static MemAllocator *pconn_fds_pool = NULL;
PconnModule * PconnModule::instance = NULL;

PconnModule::PconnModule() : pools()
{
    registerWithCacheManager();
}

PconnModule *
PconnModule::GetInstance()
{
    if (instance == NULL)
        instance = new PconnModule;

    return instance;
}

void
PconnModule::registerWithCacheManager(void)
{
    Mgr::RegisterAction("pconn",
                        "Persistent Connection Utilization Histograms",
                        DumpWrapper, 0, 1);
}

void
PconnModule::add(PconnPool *aPool)
{
    pools.insert(aPool);
}

void
PconnModule::remove(PconnPool *aPool)
{
    pools.erase(aPool);
}

void
PconnModule::dump(StoreEntry *e)
{
    typedef Pools::const_iterator PCI;
    int i = 0; // TODO: Why number pools if they all have names?
    for (PCI p = pools.begin(); p != pools.end(); ++p, ++i) {
        // TODO: Let each pool dump itself the way it wants to.
        storeAppendPrintf(e, "\n Pool %d Stats\n", i);
        (*p)->dumpHist(e);
        storeAppendPrintf(e, "\n Pool %d Hash Table\n",i);
        (*p)->dumpHash(e);
    }
}

void
PconnModule::DumpWrapper(StoreEntry *e)
{
    PconnModule::GetInstance()->dump(e);
}

