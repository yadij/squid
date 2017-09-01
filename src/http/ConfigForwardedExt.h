/*
 * Copyright (C) 1996-2017 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID__SRC_HTTP_CONFIGFORWARDEDEXT_H
#define SQUID__SRC_HTTP_CONFIGFORWARDEDEXT_H

class ConfigParser;
class Packable;

namespace Http
{

/// configuration settings for the Forwarded HTTP extension
/// see RFC 7239
class ConfigForwardedExt
{
public:
    void clear() { *this = Http::ConfigForwardedExt(); }
    void parse(ConfigParser &);
    void dump(Packable *, const char *name);

    /// the primary action to take with Forwarded headers
    enum {
        fwdTransparent, ///< relay without changes
        fwdDelete       ///< remove all Forwarded headers
    } mode = Http::ConfigForwardedExt::fwdTransparent;
};

} // namespace Http

// Legacy parsing wrappers
#define parse_http_ext_forwarded(X) (X)->parse(LegacyParser)
#define free_http_ext_forwarded(X) (X)->clear();
#define dump_http_ext_forwarded(E,N,D) (D).dump((E),(N))

#endif /* SQUID__SRC_HTTP_CONFIGFORWARDEDEXT_H */
