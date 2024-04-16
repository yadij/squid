/*
 * Copyright (C) 1996-2023 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_SRC_TIME_FORWARD_H
#define SQUID_SRC_TIME_FORWARD_H

#include <chrono>

/// Time and Date handling tools
namespace Time
{

using DefaultClock = std::chrono::system_clock;

class Engine;

using Point = std::chrono::time_point<DefaultClock>;

} // namespace Time

#endif /* SQUID_SRC_TIME_FORWARD_H */

