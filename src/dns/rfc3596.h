/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_SRC_DNS_RFC3596_H
#define SQUID_SRC_DNS_RFC3596_H

/* RFC 3596 extends RFC 1035 */
#include "dns/rfc1035.h"

ssize_t rfc3596BuildAQuery(const char *hostname,
                           char *buf,
                           size_t sz,
                           unsigned short qid,
                           rfc1035_query *);

ssize_t rfc3596BuildAAAAQuery(const char *hostname,
                              char *buf,
                              size_t sz,
                              unsigned short qid,
                              rfc1035_query *);

ssize_t rfc3596BuildPTRQuery4(const struct in_addr,
                              char *buf,
                              size_t sz,
                              unsigned short qid,
                              rfc1035_query *);

ssize_t rfc3596BuildPTRQuery6(const struct in6_addr,
                              char *buf,
                              size_t sz,
                              unsigned short qid,
                              rfc1035_query *);

/* RFC3596 library implements RFC1035 generic host interface */
ssize_t rfc3596BuildHostQuery(const char *hostname,
                              char *buf,
                              size_t sz,
                              unsigned short qid,
                              rfc1035_query * query,
                              int qtype);

/* RFC3596 section 2.1 defines new RR type AAAA as 28 */
#define RFC1035_TYPE_AAAA 28

#endif /* SQUID_SRC_DNS_RFC3596_H */

