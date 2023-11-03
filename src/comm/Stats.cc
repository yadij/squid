/*
 * Copyright (C) 1996-2023 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 05    Socket Functions */

#include "squid.h"
#include "base/PackableStream.h"
#include "comm/Stats.h"
#include "StatCounters.h"
#include "Store.h"

void
Comm::IncomingStats(StoreEntry *sentry)
{
    const auto yamlIndent = "";

    PackableStream os(*sentry);
    const auto tableRows = [&os,&yamlIndent](int, double val, double, int count){
        if (count) {
            os << yamlIndent
               << ' ' << val
               << '\t' << count
               << '\n';
        }
    };

    os << "Total number of select loops: " << statCounter.select_loops << '\n'
       << "Histogram of returned filedescriptors:\n";
    statCounter.select_fds_hist.display(tableRows);
    os << '\n';

#if USE_POLL || USE_SELECT
    yamlIndent = "  ";
    os << "Current incoming_udp_interval: " << statCounter.udp.incoming_interval << '\n'
       << "Current incoming_dns_interval: " << statCounter.dns.incoming_interval << '\n'
       << "Current incoming_tcp_interval: " << statCounter.tcp.incoming_interval << '\n'
       << '\n'
       << "Histogram of events per incoming socket type:\n"
       << yamlIndent << "ICP Messages handled per comm_poll_udp_incoming() call:\n";
    statCounter.udp.comm_incoming.display(tableRows);
    os << yamlIndent << "DNS Messages handled per comm_poll_dns_incoming() call:\n";
    statCounter.dns.comm_incoming.display(tableRows);
    os << yamlIndent << "HTTP Messages handled per comm_poll_tcp_incoming() call:\n";
    statCounter.tcp.comm_incoming.display(tableRows);
#endif
}

