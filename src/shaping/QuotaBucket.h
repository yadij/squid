/*
 * Copyright (C) 1996-2021 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef _SQUID__SRC_SHAPING_QUOTABUCKET_H
#define _SQUID__SRC_SHAPING_QUOTABUCKET_H

#if USE_DELAY_POOLS

#include "shaping/BandwidthBucket.h"
#include "shaping/forward.h"

namespace Shaping
{

/// Limits Squid-to-client bandwidth for each client
class QuotaBucket : public BandwidthBucket
{
public:
    virtual ~QuotaBucket();

    /// whether any clients are waiting for write quota
    bool hasQueue() const;

    /// has a given queue
    bool hasQueue(const Shaping::QuotaQueue *) const;

    /// client starts waiting in queue; create the queue if necessary
    unsigned int quotaEnqueue(int fd);

    /// returns the next FD reservation ID
    int quotaPeekFd() const;

    /// returns the next reservation ID to pop
    unsigned int quotaPeekReserv() const;

    /// pops queue head from queue
    void quotaDequeue();

    /// schedule commHandleWriteHelper call
    void kickQuotaQueue();

    /// either selects the head descriptor for writing or calls quotaDequeue()
    void writeOrDequeue();

    /**
     * Configure client write limiting (note:"client" here means - IP). It is called
     * by httpAccept in client_side.cc, where the initial bucket size (anInitialBurst)
     * computed, using the configured maximum bucket value and configured initial
     * bucket value (50% by default).
     *
     * \param writeSpeedLimit is speed limit configured in config for this pool
     * \param initialBurst is initial bucket size to use for this client(i.e. client can burst at first)
     * \param highWatermark is maximum bucket value
     */
    void setWriteLimiter(const int aWriteSpeedLimit, const double anInitialBurst, const double aHighWatermark);

    /* BandwidthBucket API */
    virtual int quota() override;
    virtual bool applyQuota(int &, Comm::IoCallback *) override;
    virtual void scheduleWrite(Comm::IoCallback *) override;
    virtual void onFdClosed() override;
    virtual void reduceBucket(int) override;

public:
    QuotaQueuePointer quotaQueue; ///< clients waiting for more write quota

    bool writeLimitingActive = false; ///< Is write limiter active
    bool firstTimeConnection = false; ///< is this first time connection for this client

    int rationedQuota = 0; ///< precomputed quota preserving fairness among clients
    int rationedCount = 0; ///< number of clients that will receive rationedQuota
    bool eventWaiting = false; ///< waiting for commHandleWriteHelper event to fire
};

} // namespace Shaping

#endif /* USE_DELAY_POOLS */
#endif /* _SQUID__SRC_SHAPING_QUOTABUCKET_H */
