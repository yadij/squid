/*
 * Copyright (C) 1996-2021 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef _SQUID__SRC_SHAPING_QUOTAQUEUE_H
#define _SQUID__SRC_SHAPING_QUOTAQUEUE_H

#if USE_DELAY_POOLS

#include "cbdata.h"

#include <deque>

class ClientInfo;

namespace Shaping
{

/// a queue of clients waiting for I/O quota controlled by delay pools
class QuotaQueue
{
    CBDATA_CLASS(QuotaQueue);

public:
    QuotaQueue(ClientInfo *);
    ~QuotaQueue();

    bool empty() const { return fds.empty(); }
    size_t size() const { return fds.size(); }
    int front() const { return fds.front(); }
    unsigned int enqueue(int fd);
    void dequeue();

public:
    ClientInfo *clientInfo = nullptr; ///< bucket responsible for quota maintenance

    // these counters might overflow; that is OK because they are for IDs only
    int ins = 0; ///< number of enqueue calls, used to generate a "reservation" ID
    int outs = 0; ///< number of dequeue calls, used to check the "reservation" ID

private:
    std::deque<int> fds; ///< descriptor queue
};

} // namespace Shaping

#endif /* USE_DELAY_POOLS */
#endif /* _SQUID__SRC_SHAPING_QUOTAQUEUE_H */
