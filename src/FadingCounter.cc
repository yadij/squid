/*
 * Copyright (C) 1996-2023 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "base/TextException.h"
#include "debug/Stream.h"
#include "FadingCounter.h"
#include "time/gadgets.h"

#include <cmath>

void
FadingCounter::clear()
{
    counters = {};
    lastTime = Time::DefaultClock::now();
    total = 0;
}

void
FadingCounter::configure(const Horizon &newHorizon)
{
    if (newHorizon != horizon_) {
        clear(); // for simplicity
        horizon_ = newHorizon;
        delta = horizon_.count() / counters.size(); // may become zero
    }
}

uint64_t
FadingCounter::count(uint64_t howMany)
{
    if (horizon() == Horizon::max())
        return total += howMany; // forget nothing

    if (horizon() == Horizon::zero())
        return howMany; // remember nothing

    const auto now = Time::DefaultClock::now();
    const auto past = std::chrono::duration_cast<std::chrono::milliseconds>(lastTime.time_since_epoch()) % horizon();

    if (now < lastTime) {
        clear(); // forget all values
    } else {
        const double deltas = (now - lastTime).count() / delta;
        if (deltas >= counters.size()) {
            clear(); // forget all values
        } else {
            // forget stale values, if any
            // fmod() or "past/delta" will overflow int for small deltas
            const int lastSlot = static_cast<int>(fmod(past.count(), horizon().count()) / delta);
            const int staleSlots = static_cast<int>(deltas);
            for (int i = 0, s = lastSlot + 1; i < staleSlots; ++i, ++s) {
                const int idx = s % counters.size();
                Assure(counters[idx] >= 0);
                Assure(total >= uint64_t(counters[idx]));
                total -= counters[idx];
                counters[idx] = 0;
            }
        }
    }

    // apply new information
    lastTime = now;
    const auto idx = static_cast<int>(fmod(past.count(), horizon().count()) / delta) % counters.size();
    counters[idx] += howMany;
    total += howMany;

    return total;
}
