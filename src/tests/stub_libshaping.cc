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

#include "shaping/MessageBucket.h"
namespace Shaping
{
MessageBucket::MessageBucket(const int, const int, const double, MessageDelayPool::Pointer) {STUB}
int MessageBucket::quota() STUB_RETVAL(0)
void MessageBucket::scheduleWrite(Comm::IoCallback *) STUB
void MessageBucket::reduceBucket(int) STUB
} // namespace Shaping

#include "shaping/QuotaQueue.h"
CBDATA_NAMESPACED_CLASS_INIT(Shaping, QuotaQueue);
namespace Shaping
{
Shaping::QuotaQueue::QuotaQueue(ClientInfo *) {STUB}
Shaping::QuotaQueue::~QuotaQueue() STUB
unsigned int Shaping::QuotaQueue::enqueue(int) STUB_RETVAL(0)
void Shaping::QuotaQueue::dequeue() STUB
} // namespace Shaping

#include "shaping/QuotaBucket.h"
namespace Shaping
{
QuotaBucket::~QuotaBucket() {STUB}
bool QuotaBucket::hasQueue() const STUB_RETVAL(false)
bool QuotaBucket::hasQueue(const Shaping::QuotaQueue *) const STUB_RETVAL(false)
unsigned int QuotaBucket::quotaEnqueue(int) STUB_RETVAL(0)
int QuotaBucket::quotaPeekFd() const STUB_RETVAL(-1)
unsigned int QuotaBucket::quotaPeekReserv() const STUB
void QuotaBucket::quotaDequeue() STUB
void QuotaBucket::kickQuotaQueue() STUB
void QuotaBucket::writeOrDequeue() STUB
void QuotaBucket::setWriteLimiter(const int, const double, const double) STUB
int QuotaBucket::quota() STUB_RETVAL(0)
bool QuotaBucket::applyQuota(int &, Comm::IoCallback *) STUB_RETVAL(false)
void QuotaBucket::scheduleWrite(Comm::IoCallback *) STUB
void QuotaBucket::onFdClosed() STUB
void QuotaBucket::reduceBucket(int) STUB
} // namespace Shaping

#endif /* USE_DELAY_POOLS */
