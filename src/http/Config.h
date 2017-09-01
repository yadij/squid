/*
 * Copyright (C) 1996-2017 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID__SRC_HTTP_CONFIG_H
#define SQUID__SRC_HTTP_CONFIG_H

#include "base/Packable.h"
#include "cache_cf.h"

namespace Http
{

/// configuration settings for the Forwarded HTTP extension
/// see RFC 7239
class ExtForwardedCfg
{
public:
    void clear() { *this = ExtForwardedCfg(); }
    void parse(ConfigParser &);
    void dump(Packable *, const char *name);

    /// the primary action to take with Forwarded headers
    enum {
        fwdTransparent, ///< relay without changes
        fwdDelete       ///< remove all Forwarded headers
    } mode = Http::ExtForwardedCfg::fwdTransparent;
};

/// configuration settings related to HTTP protocol
class HttpConfig
{
public:
    void parse(const char *directiveName, ConfigParser &);
    void dump(Packable *, const char *name);

    /// configuration settings for the Forwarded HTTP extension
    ExtForwardedCfg extForwarded;
};

extern HttpConfig TheConfig;

} // namespace Http

// Legacy parsing wrappers
#define parse_http_ext_forwarded(X) (X)->parse(LegacyParser)
#define free_http_ext_forwarded(X) (X)->clear();
#define dump_http_ext_forwarded(E,N,D) (D).dump((E),(N))

#endif /* SQUID__SRC_HTTP_CONFIG_H */
