/*
 * Copyright (C) 1996-2021 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef _SQUID__SRC_SHAPING_BANDWIDTHBUCKET_H
#define _SQUID__SRC_SHAPING_BANDWIDTHBUCKET_H

#if USE_DELAY_POOLS

#include "comm/forward.h"

class fde;

namespace Shaping
{

/// Base class for Squid-to-client bandwidth limiting
class BandwidthBucket
{
public:
    BandwidthBucket() = default;
    BandwidthBucket(const int speed, const int initialLevelPercent, const double sizeLimit);
    virtual ~BandwidthBucket() {}

    static BandwidthBucket *SelectBucket(fde *);

    /// \returns the number of bytes this bucket allows to write,
    /// also considering aggregates, if any. Negative quota means
    /// no limitations by this bucket.
    virtual int quota() = 0;

    /// Adjusts nleft to not exceed the current bucket quota value,
    /// if needed.
    virtual bool applyQuota(int &nleft, Comm::IoCallback *);

    /// Will plan another write call.
    virtual void scheduleWrite(Comm::IoCallback *) = 0;

    /// Performs cleanup when the related file descriptor becomes closed.
    virtual void onFdClosed() { selectWaiting = false; }

    /// Decreases the bucket level.
    virtual void reduceBucket(const int len);

    /// Whether this bucket will not do bandwidth limiting.
    bool noLimit() const { return writeSpeedLimit < 0.0; }

protected:
    /// Increases the bucket level with the writeSpeedLimit speed.
    void refillBucket();

public:
    double bucketLevel = 0.0; ///< how much can be written now
    bool selectWaiting = false; ///< is between commSetSelect and commHandleWrite

protected:
    double prevTime = 0.0; ///< previous time when we checked
    double writeSpeedLimit = -1.0; ///< Write speed limit in bytes per second.
    double bucketSizeLimit = 0.0; ///< maximum bucket size
};

} // namespace Shaping

#endif /* USE_DELAY_POOLS */
#endif /* _SQUID__SRC_SHAPING_BANDWIDTHBUCKET_H */

