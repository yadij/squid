/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_SRC_HIERARCHYLOGENTRY_H
#define SQUID_SRC_HIERARCHYLOGENTRY_H

#include "base/Stopwatch.h"
#include "comm/Connection.h"
#include "enums.h"
#include "hier_code.h"
#include "http/StatusCode.h"
#include "lookup_t.h"
#include "PingData.h"
#include "rfc2181.h"

/// Maintains peer selection details and peer I/O stats.
/// Here, "peer" is an origin server or CachePeer.
class HierarchyLogEntry
{

public:
    HierarchyLogEntry();

    /// Start recording new origin server or cache peer connection details.
    /// Call this when trying to connect to a peer.
    void resetPeerNotes(const Comm::ConnectionPointer &server, const char *requestedHost);

    /// Account for a TCP peer read. Maintains peer response time stats (%<pt).
    /// Call this after each successful peer socket read(2).
    void notePeerRead();

    /// Account for a TCP peer write. Maintains peer response time stats (%<pt).
    /// Call this after each peer socket write(2), including failed ones.
    void notePeerWrite();

    /// Estimates response generation and sending delay at the last peer.
    /// \returns whether the estimate (stored in `responseTime`) is available.
    bool peerResponseTime(struct timeval &responseTime);

public:
    hier_code code;
    char host[SQUIDHOSTNAMELEN];
    ping_data ping;
    char cd_host[SQUIDHOSTNAMELEN]; /* the host of selected by cd peer */
    lookup_t cd_lookup;     /* cd prediction: none, miss, hit */
    int n_choices;      /* #peers we selected from (cd only) */
    int n_ichoices;     /* #peers with known rtt we selected from (cd only) */

    struct timeval peer_select_start;

    struct timeval store_complete_stop;

    Http::StatusCode peer_reply_status; ///< last HTTP status code received
    Comm::ConnectionPointer tcpServer; ///< TCP/IP level details of the last peer/server connection
    int64_t bodyBytesRead;  ///< number of body bytes received from the next hop or -1

    /// cumulative time spent (so far) communicating with all peers (see %<tt)
    Stopwatch totalPeeringTime;

private:
    void clearPeerNotes();

    struct timeval peer_last_read_; ///< time of the last read from the last peer
    struct timeval peer_last_write_; ///< time of the last write to the last peer
};

#endif /* SQUID_SRC_HIERARCHYLOGENTRY_H */

