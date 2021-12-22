/*
 * Copyright (C) 1996-2021 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef _SQUID__SRC_SHAPING_DELAYBUCKET_H
#define _SQUID__SRC_SHAPING_DELAYBUCKET_H

#if USE_DELAY_POOLS

class DelaySpec;
class StoreEntry;

namespace Shaping
{

class DelayBucket
{
public:
    int const& level() const { return level_; }
    int & level() { return level_; }

    void stats(StoreEntry *) const;
    void update(DelaySpec const &, int incr);
    int bytesWanted (int min, int max) const;
    void bytesIn(int qty);
    void init(DelaySpec const &);

private:
    int level_ = 0;
};

} // namespace Shaping

#endif /* USE_DELAY_POOLS */
#endif /* _SQUID__SRC_SHAPING_DELAYBUCKET_H */
