/*
 * Copyright (C) 1996-2021 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"

#if USE_DELAY_POOLS

#define STUB_API "shaping/libshaping.la"
#include "tests/STUB.h"

#include "shaping/BandwidthBucket.h"
namespace Shaping {
BandwidthBucket::BandwidthBucket(const int, const int, const double) {STUB}
BandwidthBucket *BandwidthBucket::SelectBucket(fde *) STUB_RETVAL(nullptr)
bool BandwidthBucket::applyQuota(int &, Comm::IoCallback *) STUB_RETVAL(false)
void BandwidthBucket::reduceBucket(const int) STUB
void BandwidthBucket::refillBucket() STUB
} // namespace Shaping

#endif /* USE_DELAY_POOLS */
