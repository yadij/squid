/*
 * Copyright (C) 1996-2021 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef _SQUID__SRC_SHAPING_MESSAGEBUCKET_H
#define _SQUID__SRC_SHAPING_MESSAGEBUCKET_H

#if USE_DELAY_POOLS

#include "base/RefCount.h"
#include "comm/forward.h"
#include "MessageDelayPools.h"
#include "shaping/BandwidthBucket.h"

namespace Shaping
{

/// Limits Squid-to-client bandwidth for each matching response
class MessageBucket : public RefCountable, public BandwidthBucket
{
    MEMPROXY_CLASS(MessageBucket);

public:
    MessageBucket(const int speed, const int initialLevelPercent, const double sizeLimit, MessageDelayPool::Pointer);

    /* Shaping::BandwidthBucket API */
    virtual int quota() override;
    virtual void scheduleWrite(Comm::IoCallback *) override;
    virtual void reduceBucket(int len) override;

private:
    MessageDelayPool::Pointer theAggregate;
};

} // namespace Shaping

#endif /* USE_DELAY_POOLS */
#endif /* _SQUID__SRC_SHAPING_MESSAGEBUCKET_H */
