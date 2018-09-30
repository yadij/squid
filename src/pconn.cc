/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 48    Persistent Connections */

#include "squid.h"
#include "CachePeer.h"
#include "comm.h"
#include "comm/Connection.h"
#include "comm/IdleConnList.h"
#include "comm/Read.h"
#include "fd.h"
#include "fde.h"
#include "globals.h"
#include "mgr/Registration.h"
#include "neighbors.h"
#include "pconn.h"
#include "PeerPoolMgr.h"
#include "SquidConfig.h"
#include "Store.h"

//TODO: re-attach to MemPools. WAS: static MemAllocator *pconn_fds_pool = NULL;
PconnModule * PconnModule::instance = NULL;

/* ========== PconnPool PRIVATE FUNCTIONS ============================================ */

const char *
PconnPool::key(const Comm::ConnectionPointer &destLink, const char *domain)
{
    LOCAL_ARRAY(char, buf, SQUIDHOSTNAMELEN * 3 + 10);

    destLink->remote.toUrl(buf, SQUIDHOSTNAMELEN * 3 + 10);
    if (domain) {
        const int used = strlen(buf);
        snprintf(buf+used, SQUIDHOSTNAMELEN * 3 + 10-used, "/%s", domain);
    }

    debugs(48,6,"PconnPool::key(" << destLink << ", " << (domain?domain:"[no domain]") << ") is {" << buf << "}" );
    return buf;
}

void
PconnPool::dumpHist(StoreEntry * e) const
{
    storeAppendPrintf(e,
                      "%s persistent connection counts:\n"
                      "\n"
                      "\t Requests\t Connection Count\n"
                      "\t --------\t ----------------\n",
                      descr);

    for (int i = 0; i < PCONN_HIST_SZ; ++i) {
        if (hist[i] == 0)
            continue;

        storeAppendPrintf(e, "\t%d\t%d\n", i, hist[i]);
    }
}

void
PconnPool::dumpHash(StoreEntry *e) const
{
    hash_table *hid = table;
    hash_first(hid);

    int i = 0;
    for (hash_link *walker = hash_next(hid); walker; walker = hash_next(hid)) {
        storeAppendPrintf(e, "\t item %d:\t%s\n", i, (char *)(walker->key));
        ++i;
    }
}

/* ========== PconnPool PUBLIC FUNCTIONS ============================================ */

PconnPool::PconnPool(const char *aDescr, const CbcPointer<PeerPoolMgr> &aMgr):
    table(NULL), descr(aDescr),
    mgr(aMgr),
    theCount(0)
{
    int i;
    table = hash_create((HASHCMP *) strcmp, 229, hash_string);

    for (i = 0; i < PCONN_HIST_SZ; ++i)
        hist[i] = 0;

    PconnModule::GetInstance()->add(this);
}

static void
DeleteIdleConnList(void *hashItem)
{
    delete static_cast<IdleConnList*>(hashItem);
}

PconnPool::~PconnPool()
{
    PconnModule::GetInstance()->remove(this);
    hashFreeItems(table, &DeleteIdleConnList);
    hashFreeMemory(table);
    descr = NULL;
}

void
PconnPool::push(const Comm::ConnectionPointer &conn, const char *domain)
{
    if (fdUsageHigh()) {
        debugs(48, 3, HERE << "Not many unused FDs");
        conn->close();
        return;
    } else if (shutting_down) {
        conn->close();
        debugs(48, 3, HERE << "Squid is shutting down. Refusing to do anything");
        return;
    }
    // TODO: also close used pconns if we exceed peer max-conn limit

    const char *aKey = key(conn, domain);
    IdleConnList *list = (IdleConnList *) hash_lookup(table, aKey);

    if (list == NULL) {
        list = new IdleConnList(aKey, this);
        debugs(48, 3, "new IdleConnList for {" << hashKeyStr(list) << "}" );
        hash_join(table, list);
    } else {
        debugs(48, 3, "found IdleConnList for {" << hashKeyStr(list) << "}" );
    }

    list->push(conn);
    assert(!comm_has_incomplete_write(conn->fd));

    LOCAL_ARRAY(char, desc, FD_DESC_SZ);
    snprintf(desc, FD_DESC_SZ, "Idle server: %s", aKey);
    fd_note(conn->fd, desc);
    debugs(48, 3, HERE << "pushed " << conn << " for " << aKey);

    // successful push notifications resume multi-connection opening sequence
    notifyManager("push");
}

Comm::ConnectionPointer
PconnPool::pop(const Comm::ConnectionPointer &dest, const char *domain, bool keepOpen)
{

    const char * aKey = key(dest, domain);

    IdleConnList *list = (IdleConnList *)hash_lookup(table, aKey);
    if (list == NULL) {
        debugs(48, 3, HERE << "lookup for key {" << aKey << "} failed.");
        // failure notifications resume standby conn creation after fdUsageHigh
        notifyManager("pop failure");
        return Comm::ConnectionPointer();
    } else {
        debugs(48, 3, "found " << hashKeyStr(list) <<
               (keepOpen ? " to use" : " to kill"));
    }

    /* may delete list */
    Comm::ConnectionPointer popped = list->findUseable(dest);
    if (!keepOpen && Comm::IsConnOpen(popped))
        popped->close();

    // successful pop notifications replenish standby connections pool
    notifyManager("pop");
    return popped;
}

void
PconnPool::notifyManager(const char *reason)
{
    if (mgr.valid())
        PeerPoolMgr::Checkpoint(mgr, reason);
}

void
PconnPool::closeN(int n)
{
    hash_table *hid = table;
    hash_first(hid);

    // close N connections, one per list, to treat all lists "fairly"
    for (int i = 0; i < n && count(); ++i) {

        hash_link *current = hash_next(hid);
        if (!current) {
            hash_first(hid);
            current = hash_next(hid);
            Must(current); // must have one because the count() was positive
        }

        // may delete current
        static_cast<IdleConnList*>(current)->closeN(1);
    }
}

void
PconnPool::unlinkList(IdleConnList *list)
{
    theCount -= list->count();
    assert(theCount >= 0);
    hash_remove_link(table, list);
}

void
PconnPool::noteUses(int uses)
{
    if (uses >= PCONN_HIST_SZ)
        uses = PCONN_HIST_SZ - 1;

    ++hist[uses];
}

/* ========== PconnModule ============================================ */

/*
 * This simple class exists only for the cache manager
 */

PconnModule::PconnModule(): pools()
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

