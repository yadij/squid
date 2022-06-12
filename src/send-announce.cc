/*
 * Copyright (C) 1996-2022 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 27    Cache Announcer */

#include "squid.h"
#include "anyp/PortCfg.h"
#include "base/RunnersRegistry.h"
#include "comm/Connection.h"
#include "event.h"
#include "fd.h"
#include "fde.h"
#include "globals.h"
#include "ICP.h"
#include "ipcache.h"
#include "sbuf/Stream.h"
#include "SquidConfig.h"
#include "tools.h"

#include <cstdio>

static void
appendFileContent(SBufStream &os, const char *file)
{
    if (auto *fd = fopen(file,"r")) {
        while (!feof(fd)) {
            static char tbuf[BUFSIZ] = {};
            const auto n = fread(tbuf, sizeof(tbuf)-1, 1, fd);
            if (n > 0) {
                os.write(tbuf, n);
            } else {
                int xerrno = errno;
                debugs(50, DBG_IMPORTANT, "send_announce: " << file << ": " << xstrerr(xerrno));
                break;
            }
        }
        fclose(fd);
    } else {
        int xerrno = errno;
        debugs(50, DBG_IMPORTANT, "send_announce: " << file << ": " << xstrerr(xerrno));
    }
}

static void
send_announce(const ipcache_addrs *ia, const Dns::LookupDetails &, void *)
{
    char *host = Config.Announce.host;

    if (!ia) {
        debugs(27, DBG_IMPORTANT, "ERROR: send_announce: Unknown host '" << host << "'");
        return;
    }

    debugs(27, DBG_IMPORTANT, "Sending Announcement to " << host);

    SBufStream os;
    os << "cache_version SQUID/" << version_string << "\n";
    os << "Running on " << getMyHostname() << " " << getMyPort() << " " << Config.Port.icp << "\n";
    if (Config.adminEmail)
        os << "cache_admin: " << Config.adminEmail << "\n";
    os << "generated " << squid_curtime << " [" << Time::FormatHttpd(squid_curtime) << "]\n";

    if (const auto file = Config.Announce.file)
        appendFileContent(os, file);

    Ip::Address S = ia->current();
    S.port(Config.Announce.port);
    assert(Comm::IsConnOpen(icpOutgoingConn));

    auto sendBuf = os.buf();

    if (comm_udp_sendto(icpOutgoingConn->fd, S, sendBuf.rawContent(), sendBuf.length()) < 0) {
        int xerrno = errno;
        debugs(27, DBG_IMPORTANT, "ERROR: Failed to announce to " << S << " from " << icpOutgoingConn->local << ": " << xstrerr(xerrno));
    }
}

static void
start_announce(void *)
{
    if (!Config.onoff.announce)
        return;

    if (!Comm::IsConnOpen(icpOutgoingConn))
        return;

    ipcache_nbgethostbyname(Config.Announce.host, send_announce, nullptr);

    eventAdd("send_announce", start_announce, nullptr, static_cast<double>(Config.Announce.period), 1);
}

/// launches send-announce event
class SendAnnounceRr: public RegisteredRunner
{
public:
    /* RegisteredRunner API */
    virtual void finalizeConfig() {
        eventDelete(start_announce, nullptr);
        if (Config.onoff.announce)
            eventAdd("send_announce", start_announce, nullptr, Config.Announce.period, 1);
    }

    virtual void startShutdown() {
        eventDelete(start_announce, nullptr);
    }
};

RunnerRegistrationEntry(SendAnnounceRr);
