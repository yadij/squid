/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 48    Persistent Connections */

#include "squid.h"
#include "comm/Connection.h"
#include "comm/IdleConnList.h"
#include "comm/PconnPool.h"
#include "comm/PconnModule.h"
#include "fd.h"
#include "PeerPoolMgr.h"
#include "Store.h"

/**
 * Generate the hash key for storing or looking up the provided connection
 * object for use with the given domain.
 *
 * Always produces a value. The value produced is not guaranteed to be
 * valid past any subsequent call to this function.
 */
const PconnPool::key_type
PconnPool::Key(const Comm::ConnectionPointer &destLink, const char *domain)
{
    LOCAL_ARRAY(char, buf, SQUIDHOSTNAMELEN * 3 + 10);

    destLink->remote.toUrl(buf, SQUIDHOSTNAMELEN * 3 + 10);
    if (domain) {
        const int used = strlen(buf);
        snprintf(buf+used, SQUIDHOSTNAMELEN * 3 + 10-used, "/%s", domain);
    }

    debugs(48,6,"key(" << destLink << ", " << (domain?domain:"[no domain]") << ") is {" << buf << "}" );
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
    size_t pos = 0;
    for (const auto itr : data) {
        e->appendf("\t item %" PRIuSIZE":\t%s\n", pos, itr.first);
        ++pos;
    }
}

PconnPool::PconnPool(const char *aDescr, const CbcPointer<PeerPoolMgr> &aMgr):
    descr(aDescr),
    mgr(aMgr),
    theCount(0)
{
    for (int i = 0; i < PCONN_HIST_SZ; ++i)
        hist[i] = 0;

    PconnModule::GetInstance()->add(this);
}

PconnPool::~PconnPool()
{
    PconnModule::GetInstance()->remove(this);
    // TODO make data store Pointer instead of raw-pointers
    for (auto i : data) {
        delete i.second;
        i.second = nullptr;
    }
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

    const auto aKey = Key(conn, domain);
    auto &list = data[aKey];
    if (!list) {
        list = new IdleConnList(aKey, this);
        debugs(48, 3, "new IdleConnList for {" << aKey << "}" );
    } else {
        debugs(48, 3, "found IdleConnList for {" << aKey << "}" );
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
    const auto aKey = Key(dest, domain);

    const auto list = data.find(aKey);
    if (list == data.end()) {
        debugs(48, 3, HERE << "lookup for key {" << aKey << "} failed.");
        // failure notifications resume standby conn creation after fdUsageHigh
        notifyManager("pop failure");
        return Comm::ConnectionPointer();
    } else {
        debugs(48, 3, "found " << aKey << (keepOpen ? " to use" : " to kill"));
    }

    /* may delete list */
    auto popped = list->second->findUseable(dest);
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
    if (count() < 1)
        return; // nothing to close

    // close N connections, one per list, to treat all lists "fairly"
    do {
        for (auto current : data) {
            if (n == 0)
                break;

            if (!current.second) // map may have entries which are nullptr
                continue;

            // may delete current
            current.second->closeN(1);
            --n;
        }
    } while (n > 0 && count() > 0);
}

void
PconnPool::unlinkList(IdleConnList *list)
{
    theCount -= list->count();
    assert(theCount >= 0);
    // TODO: see if we can get the key quicker from anywhere
    for (auto itr : data) {
        if (itr.second == list) {
            data.erase(itr.first);
            return;
        }
    }
}

void
PconnPool::noteUses(int uses)
{
    if (uses >= PCONN_HIST_SZ)
        uses = PCONN_HIST_SZ - 1;

    ++hist[uses];
}

