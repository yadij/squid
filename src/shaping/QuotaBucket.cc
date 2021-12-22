/*
 * Copyright (C) 1996-2021 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"

#if USE_DELAY_POOLS

#include "ClientInfo.h"
#include "comm/IoCallback.h"
#include "comm/Write.h"
#include "fde.h"
#include "shaping/QuotaBucket.h"
#include "shaping/QuotaQueue.h"
#include "SquidTime.h"

#include <cmath>

Shaping::QuotaBucket::~QuotaBucket()
{
    if (quotaQueue.valid()) {
        quotaQueue->clientInfo = nullptr;
        delete quotaQueue.get(); // invalidates cbdata, cancelling any pending kicks
    }
}

bool
Shaping::QuotaBucket::hasQueue() const
{
    assert(quotaQueue.valid());
    return !quotaQueue->empty();
}

bool
Shaping::QuotaBucket::hasQueue(const Shaping::QuotaQueue *q) const
{
    assert(quotaQueue.valid());
    return quotaQueue.get() == q;
}

/// returns the first descriptor to be dequeued
int
Shaping::QuotaBucket::quotaPeekFd() const
{
    assert(quotaQueue.valid());
    return quotaQueue->front();
}

/// returns the reservation ID of the first descriptor to be dequeued
unsigned int
Shaping::QuotaBucket::quotaPeekReserv() const
{
    assert(quotaQueue.valid());
    return quotaQueue->outs + 1;
}

/// queues a given fd, creating the queue if necessary; returns reservation ID
unsigned int
Shaping::QuotaBucket::quotaEnqueue(int fd)
{
    assert(quotaQueue.valid());
    return quotaQueue->enqueue(fd);
}

/// removes queue head
void
Shaping::QuotaBucket::quotaDequeue()
{
    assert(quotaQueue.valid());
    quotaQueue->dequeue();
}

// called when the queue is done waiting for the client bucket to fill
static void
quotaHandleWriteHelper(void * data)
{
    auto *queue = static_cast<Shaping::QuotaQueue*>(data);
    assert(queue);

    ClientInfo *clientInfo = queue->clientInfo;
    // ClientInfo invalidates queue if freed, so if we got here through,
    // evenAdd cbdata protections, everything should be valid and consistent
    assert(clientInfo);
    assert(clientInfo->hasQueue());
    assert(clientInfo->hasQueue(queue));
    assert(clientInfo->eventWaiting);
    clientInfo->eventWaiting = false;

    do {
        clientInfo->writeOrDequeue();
        if (clientInfo->selectWaiting)
            return;
    } while (clientInfo->hasQueue());

    debugs(77, 3, "emptied queue");
}

void
Shaping::QuotaBucket::kickQuotaQueue()
{
    if (!eventWaiting && !selectWaiting && hasQueue()) {
        // wait at least a second if the bucket is empty
        const double delay = (bucketLevel < 1.0) ? 1.0 : 0.0;
        eventAdd("quotaHandleWriteHelper", &quotaHandleWriteHelper, quotaQueue.get(), delay, 0, true);
        eventWaiting = true;
    }
}

void
Shaping::QuotaBucket::writeOrDequeue()
{
    assert(!selectWaiting);
    const auto head = quotaPeekFd();
    const auto &headFde = fd_table[head];
    CallBack(headFde.codeContext, [&] {
        const auto ccb = COMMIO_FD_WRITECB(head);
        // check that the head descriptor is still relevant
        if (headFde.clientInfo == this &&
                quotaPeekReserv() == ccb->quotaQueueReserv &&
                !headFde.closing()) {

            // wait for the head descriptor to become ready for writing
            Comm::SetSelect(head, COMM_SELECT_WRITE, Comm::HandleWrite, ccb, 0);
            selectWaiting = true;
        } else {
            quotaDequeue(); // remove the no longer relevant descriptor
        }
    });
}

/// calculates how much to write for a single dequeued client
int
Shaping::QuotaBucket::quota()
{
    /* If we have multiple clients and give full bucketSize to each client then
     * clt1 may often get a lot more because clt1->clt2 time distance in the
     * select(2) callback order may be a lot smaller than cltN->clt1 distance.
     * We divide quota evenly to be more fair. */

    if (!rationedCount) {
        rationedCount = quotaQueue->size() + 1;

        // The delay in ration recalculation _temporary_ deprives clients from
        // bytes that should have trickled in while rationedCount was positive.
        refillBucket();

        // Rounding errors do not accumulate here, but we round down to avoid
        // negative bucket sizes after write with rationedCount=1.
        rationedQuota = static_cast<int>(floor(bucketLevel/rationedCount));
        debugs(77, 5, "new rationedQuota: " << rationedQuota << '*' << rationedCount);
    }

    --rationedCount;
    debugs(77, 7, "rationedQuota: " << rationedQuota << " rations remaining: " << rationedCount);

    // update 'last seen' time to prevent clientdb GC from dropping us
    quotaQueue->clientInfo->last_seen = squid_curtime;
    return rationedQuota;
}

bool
Shaping::QuotaBucket::applyQuota(int &nleft, Comm::IoCallback *state)
{
    assert(hasQueue());
    assert(quotaPeekFd() == state->conn->fd);
    quotaDequeue(); // we will write or requeue below
    if (nleft > 0 && !Shaping::BandwidthBucket::applyQuota(nleft, state)) {
        state->quotaQueueReserv = quotaEnqueue(state->conn->fd);
        kickQuotaQueue();
        return false;
    }
    return true;
}

void
Shaping::QuotaBucket::scheduleWrite(Comm::IoCallback *state)
{
    if (writeLimitingActive) {
        state->quotaQueueReserv = quotaEnqueue(state->conn->fd);
        kickQuotaQueue();
    }
}

void
Shaping::QuotaBucket::onFdClosed()
{
    Shaping::BandwidthBucket::onFdClosed();
    // kick queue or it will get stuck as commWriteHandle is not called
    kickQuotaQueue();
}

void
Shaping::QuotaBucket::reduceBucket(const int len)
{
    if (len > 0)
        Shaping::BandwidthBucket::reduceBucket(len);
    // even if we wrote nothing, we were served; give others a chance
    kickQuotaQueue();
}

void
Shaping::QuotaBucket::setWriteLimiter(const int aWriteSpeedLimit, const double anInitialBurst, const double aHighWatermark)
{
    // set or possibly update traffic shaping parameters
    writeLimitingActive = true;
    writeSpeedLimit = aWriteSpeedLimit;
    bucketSizeLimit = aHighWatermark;

    // but some members should only be set once for a newly activated bucket
    if (firstTimeConnection) {
        firstTimeConnection = false;

        assert(!selectWaiting);
        assert(!quotaQueue);
        quotaQueue = new Shaping::QuotaQueue(dynamic_cast<ClientInfo *>(this));

        bucketLevel = anInitialBurst;
        prevTime = current_dtime;
    }

    debugs(77,5, "Write limits for " << quotaQueue->clientInfo->addr <<
           " speed=" << aWriteSpeedLimit << " burst=" << anInitialBurst <<
           " highwatermark=" << aHighWatermark);
}

#endif /* USE_DELAY_POOLS */
