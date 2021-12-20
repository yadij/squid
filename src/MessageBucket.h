/*
 * Copyright (C) 1996-2021 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef MESSAGEBUCKET_H
#define MESSAGEBUCKET_H

#if USE_DELAY_POOLS

#include "base/RefCount.h"
#include "comm/forward.h"
#include "MessageDelayPools.h"
#include "shaping/BandwidthBucket.h"

/// Limits Squid-to-client bandwidth for each matching response
class MessageBucket : public RefCountable, public Shaping::BandwidthBucket
{
    MEMPROXY_CLASS(MessageBucket);

public:
    typedef RefCount<MessageBucket> Pointer;

    MessageBucket(const int speed, const int initialLevelPercent, const double sizeLimit, MessageDelayPool::Pointer pool);

    /* Shaping::BandwidthBucket API */
    virtual int quota() override;
    virtual void scheduleWrite(Comm::IoCallback *state) override;
    virtual void reduceBucket(int len) override;

private:
    MessageDelayPool::Pointer theAggregate;
};

#endif /* USE_DELAY_POOLS */

#endif

