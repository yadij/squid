/*
 * Copyright (C) 1996-2021 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID__SRC_CLIENTINFO_H
#define SQUID__SRC_CLIENTINFO_H

#include "base/ByteCounter.h"
#include "cbdata.h"
#include "enums.h"
#include "hash.h"
#include "ip/Address.h"
#include "LogTags.h"
#include "mem/forward.h"
#include "shaping/QuotaBucket.h"
#include "shaping/QuotaQueue.h"
#include "typedefs.h"

class ClientInfo : public hash_link
#if USE_DELAY_POOLS
    , public Shaping::QuotaBucket
#endif
{
    MEMPROXY_CLASS(ClientInfo);

public:
    explicit ClientInfo(const Ip::Address &);
    ~ClientInfo();

    Ip::Address addr;

    struct Protocol {
        Protocol() : n_requests(0) {
            memset(result_hist, 0, sizeof(result_hist));
        }

        int result_hist[LOG_TYPE_MAX];
        int n_requests;
        ByteCounter kbytes_in;
        ByteCounter kbytes_out;
        ByteCounter hit_kbytes_out;
    } Http, Icp;

    struct Cutoff {
        Cutoff() : time(0), n_req(0), n_denied(0) {}

        time_t time;
        int n_req;
        int n_denied;
    } cutoff;
    int n_established;          /* number of current established connections */
    time_t last_seen;
};

#endif

