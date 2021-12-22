/*
 * Copyright (C) 1996-2021 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID__SRC_CLIENTINFO_H
#define SQUID__SRC_CLIENTINFO_H

#include "base/ByteCounter.h"
#include "cbdata.h"
#include "enums.h"
#include "hash.h"
#include "ip/Address.h"
#include "LogTags.h"
#include "mem/forward.h"
#include "shaping/BandwidthBucket.h"
#include "shaping/QuotaQueue.h"
#include "typedefs.h"

class ClientInfo : public hash_link
#if USE_DELAY_POOLS
    , public Shaping::BandwidthBucket
#endif
{
    MEMPROXY_CLASS(ClientInfo);

public:
    explicit ClientInfo(const Ip::Address &);
    ~ClientInfo();

    Ip::Address addr;

    struct Protocol {
        Protocol() : n_requests(0) {
            memset(result_hist, 0, sizeof(result_hist));
        }

        int result_hist[LOG_TYPE_MAX];
        int n_requests;
        ByteCounter kbytes_in;
        ByteCounter kbytes_out;
        ByteCounter hit_kbytes_out;
    } Http, Icp;

    struct Cutoff {
        Cutoff() : time(0), n_req(0), n_denied(0) {}

        time_t time;
        int n_req;
        int n_denied;
    } cutoff;
    int n_established;          /* number of current established connections */
    time_t last_seen;
#if USE_DELAY_POOLS
    bool writeLimitingActive; ///< Is write limiter active
    bool firstTimeConnection;///< is this first time connection for this client

    Shaping::QuotaQueue *quotaQueue = nullptr; ///< clients waiting for more write quota
    int rationedQuota; ///< precomputed quota preserving fairness among clients
    int rationedCount; ///< number of clients that will receive rationedQuota
    bool eventWaiting; ///< waiting for commHandleWriteHelper event to fire

    // all those functions access Comm fd_table and are defined in comm.cc
    bool hasQueue() const;  ///< whether any clients are waiting for write quota
    bool hasQueue(const Shaping::QuotaQueue*) const;  ///< has a given queue
    unsigned int quotaEnqueue(int fd); ///< client starts waiting in queue; create the queue if necessary
    int quotaPeekFd() const; ///< returns the next fd reservation
    unsigned int quotaPeekReserv() const; ///< returns the next reserv. to pop
    void quotaDequeue(); ///< pops queue head from queue
    void kickQuotaQueue(); ///< schedule commHandleWriteHelper call
    /// either selects the head descriptor for writing or calls quotaDequeue()
    void writeOrDequeue();

    /* Shaping::BandwidthBucket API */
    virtual int quota() override; ///< allocate quota for a just dequeued client
    virtual bool applyQuota(int &nleft, Comm::IoCallback *state) override;
    virtual void scheduleWrite(Comm::IoCallback *state) override;
    virtual void onFdClosed() override;
    virtual void reduceBucket(int len) override;

    void quotaDumpQueue(); ///< dumps quota queue for debugging

    /**
     * Configure client write limiting (note:"client" here means - IP). It is called
     * by httpAccept in client_side.cc, where the initial bucket size (anInitialBurst)
     * computed, using the configured maximum bucket value and configured initial
     * bucket value(50% by default).
     *
     * \param  writeSpeedLimit is speed limit configured in config for this pool
     * \param  initialBurst is initial bucket size to use for this client(i.e. client can burst at first)
     *  \param highWatermark is maximum bucket value
     */
    void setWriteLimiter(const int aWriteSpeedLimit, const double anInitialBurst, const double aHighWatermark);
#endif /* USE_DELAY_POOLS */
};

#endif

