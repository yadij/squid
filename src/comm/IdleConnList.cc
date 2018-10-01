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
#include "comm/Read.h"
#include "fde.h"
#include "pconn.h"
#include "SquidConfig.h"

/// pconn set size, increase for better memcache hit rate
#define PCONN_FDS_SZ    8

CBDATA_CLASS_INIT(IdleConnList);

IdleConnList::IdleConnList(const char *aKey, PconnPool *thePool) :
    parent_(thePool)
{
    //Initialize hash_link members
    key = xstrdup(aKey);

    theList_.reserve(PCONN_FDS_SZ);

    registerRunner();

// TODO: re-attach to MemPools. WAS: theList = (?? *)pconn_fds_pool->alloc();
}

IdleConnList::~IdleConnList()
{
    if (parent_)
        parent_->unlinkList(this);

    if (count()) {
        parent_ = nullptr; // prevent reentrant notifications and deletions
        closeN(count());
    }

    xfree(key);
}

/** Search the list. Matches by FD socket number.
 * Performed from the end of list where newest entries are.
 *
 * \retval <0   The connection is not listed
 * \retval >=0  The connection array index
 */
int
IdleConnList::findIndexOf(const Comm::ConnectionPointer &conn) const
{
    for (int index = count() - 1; index >= 0; --index) {
        if (conn->fd == theList_[index]->fd) {
            debugs(48, 3, "found " << conn << " at index " << index);
            return index;
        }
    }

    debugs(48, 2, conn << " NOT FOUND!");
    return -1;
}

/** Remove the entry at specified index.
 * May perform a shuffle of list entries to fill the gap.
 * \retval false The index is not an in-use entry.
 */
bool
IdleConnList::removeAt(size_t index)
{
    if (index >= count())
        return false;

    theList_.erase(theList_.begin()+index);

    if (parent_) {
        parent_->noteConnectionRemoved();
        if (count() == 0) {
            debugs(48, 3, "deleting " << hashKeyStr(this));
            delete this;
        }
    }

    return true;
}

// almost a duplicate of removeFD. But drops multiple entries.
void
IdleConnList::closeN(size_t n)
{
    if (n < 1) {
        debugs(48, 2, "Nothing to do.");
        return;
    } else if (n >= count()) {
        debugs(48, 2, "Closing all entries.");
        for (auto conn : theList_) {
            clearHandlers(conn);
            conn->close();
            if (parent_)
                parent_->noteConnectionRemoved();
        }
        theList_.clear();
    } else { //if (n < size_)
        debugs(48, 2, "Closing " << n << " of " << count() << " entries.");

        // ensure the first N entries are closed
        for (size_t index = 0; index < n; ++index) {
            const Comm::ConnectionPointer conn = theList_[index];
            theList_[index] = nullptr;
            clearHandlers(conn);
            conn->close();
            if (parent_)
                parent_->noteConnectionRemoved();
        }
        // erase the now closed N entries
        theList_.erase(theList_.begin(), theList_.begin()+n);
    }

    if (parent_ && count() == 0) {
        debugs(48, 3, "deleting " << hashKeyStr(this));
        delete this;
    }
}

void
IdleConnList::clearHandlers(const Comm::ConnectionPointer &conn)
{
    debugs(48, 3, "removing close handler for " << conn);
    comm_read_cancel(conn->fd, IdleConnList::Read, this);
    commUnsetConnTimeout(conn);
}

void
IdleConnList::push(const Comm::ConnectionPointer &conn)
{
    theList_.push_back(conn);

    if (parent_)
        parent_->noteConnectionAdded();

    AsyncCall::Pointer readCall = commCbCall(5,4, "IdleConnList::Read",
                                  CommIoCbPtrFun(IdleConnList::Read, this));
    comm_read(conn, fakeReadBuf_, sizeof(fakeReadBuf_), readCall);
    AsyncCall::Pointer timeoutCall = commCbCall(5,4, "IdleConnList::Timeout",
                                     CommTimeoutCbPtrFun(IdleConnList::Timeout, this));
    commSetConnTimeout(conn, conn->timeLeft(Config.Timeout.serverIdlePconn), timeoutCall);
}

/// Determine whether an entry in the idle list is available for use.
/// Returns false if the entry is unset, closed or closing.
bool
IdleConnList::isAvailable(size_t i) const
{
    const Comm::ConnectionPointer &conn = theList_[i];

    // connection already closed. useless.
    if (!Comm::IsConnOpen(conn))
        return false;

    // our connection early-read/close handler is scheduled to run already. unsafe
    if (!COMMIO_FD_READCB(conn->fd)->active())
        return false;

    return true;
}

Comm::ConnectionPointer
IdleConnList::pop()
{
    for (size_t i = count()-1; i >= 0; --i) {

        if (!isAvailable(i))
            continue;

        // our connection timeout handler is scheduled to run already. unsafe for now.
        // TODO: cancel the pending timeout callback and allow re-use of the conn.
        if (!fd_table[theList_[i]->fd].timeoutHandler)
            continue;

        // finally, a match. pop and return it.
        Comm::ConnectionPointer result = theList_[i];
        clearHandlers(result);
        /* may delete this */
        removeAt(i);
        return result;
    }

    return Comm::ConnectionPointer();
}

/*
 * XXX this routine isn't terribly efficient - if there's a pending
 * read event (which signifies the fd will close in the next IO loop!)
 * we ignore the FD and move onto the next one. This means, as an example,
 * if we have a lot of FDs open to a very popular server and we get a bunch
 * of requests JUST as they timeout (say, it shuts down) we'll be wasting
 * quite a bit of CPU. Just keep it in mind.
 */
Comm::ConnectionPointer
IdleConnList::findUseable(const Comm::ConnectionPointer &aKey)
{
    assert(theList_.size());

    // small optimization: do the constant bool tests only once.
    const bool keyCheckAddr = !aKey->local.isAnyAddr();
    const bool keyCheckPort = aKey->local.port() > 0;

    for (int i = count()-1; i >= 0; --i) {

        if (!isAvailable(i))
            continue;

        // local end port is required, but does not match.
        if (keyCheckPort && aKey->local.port() != theList_[i]->local.port())
            continue;

        // local address is required, but does not match.
        if (keyCheckAddr && aKey->local.matchIPAddr(theList_[i]->local) != 0)
            continue;

        // our connection timeout handler is scheduled to run already. unsafe for now.
        // TODO: cancel the pending timeout callback and allow re-use of the conn.
        if (!fd_table[theList_[i]->fd].timeoutHandler)
            continue;

        // finally, a match. pop and return it.
        Comm::ConnectionPointer result = theList_[i];
        clearHandlers(result);
        /* may delete this */
        removeAt(i);
        return result;
    }

    return Comm::ConnectionPointer();
}

/* might delete list */
void
IdleConnList::findAndClose(const Comm::ConnectionPointer &conn)
{
    const int index = findIndexOf(conn);
    if (index >= 0) {
        if (parent_)
            parent_->notifyManager("idle conn closure");
        clearHandlers(conn);
        /* might delete this */
        removeAt(index);
        conn->close();
    }
}

void
IdleConnList::Read(const Comm::ConnectionPointer &conn, char *, size_t len, Comm::Flag flag, int, void *data)
{
    debugs(48, 3, len << " bytes from " << conn);

    if (flag == Comm::ERR_CLOSING) {
        debugs(48, 3, "Comm::ERR_CLOSING from " << conn);
        /* Bail out on Comm::ERR_CLOSING - may happen when shutdown aborts our idle FD */
        return;
    }

    auto *list = static_cast<IdleConnList *>(data);
    /* may delete list/data */
    list->findAndClose(conn);
}

void
IdleConnList::Timeout(const CommTimeoutCbParams &io)
{
    debugs(48, 3, io.conn);
    auto *list = static_cast<IdleConnList *>(io.data);
    /* may delete list/data */
    list->findAndClose(io.conn);
}

void
IdleConnList::endingShutdown()
{
    closeN(count());
}

