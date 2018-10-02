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
#include "comm/PconnPool.h"
#include "mgr/Registration.h"
// for StoreEntry
#include "Store.h"

PconnModule::PconnModule()
{
    Mgr::RegisterAction("pconn",
                        "Persistent Connection Utilization Histograms",
                        DumpWrapper, 0, 1);
}

PconnModule *
PconnModule::GetInstance()
{
    static PconnModule instance;
    return &instance;
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
    int i = 0; // TODO: Why number pools if they all have names?
    for (const auto p : pools) {
        // TODO: Let each pool dump itself the way it wants to.
        e->appendf("\n Pool %d Stats\n", i);
        p->dumpHist(e);
        e->appendf("\n Pool %d Hash Table\n", i);
        p->dumpHash(e);
        ++i;
    }
}

void
PconnModule::DumpWrapper(StoreEntry *e)
{
    PconnModule::GetInstance()->dump(e);
}

