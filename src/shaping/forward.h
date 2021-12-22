/*
 * Copyright (C) 1996-2021 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef _SQUID__SRC_SHAPING_FORWARD_H
#define _SQUID__SRC_SHAPING_FORWARD_H

#if USE_DELAY_POOLS

#include "base/RefCount.h"

/// Traffic Shaping (aka. Delay Pools)
namespace Shaping
{

class BandwidthBucket;

class MessageBucket;
typedef RefCount<MessageBucket> MessageBucketPointer;

class QuotaQueue;

} // namespace Shaping

#endif /* USE_DELAY_POOLS */
#endif /* _SQUID__SRC_SHAPING_FORWARD_H */
