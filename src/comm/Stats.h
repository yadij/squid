/*
 * Copyright (C) 1996-2023 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID__SRC_COMM_STATS_H
#define SQUID__SRC_COMM_STATS_H

class StoreEntry;

namespace Comm
{

/// produce Cache Manager report for comm_*_incoming
void IncomingStats(StoreEntry *);

} // namespace Comm

#endif /* SQUID__SRC_COMM_STATS_H */
