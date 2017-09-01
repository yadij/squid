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
#include "http/ConfigForwardedExt.h"

namespace Http
{

/// configuration settings related to HTTP protocol
class HttpConfig
{
public:
    void parse(const char *directiveName, ConfigParser &);
    void dump(Packable *, const char *name);

    /// configuration settings for the Forwarded HTTP extension
    Http::ConfigForwardedExt extForwarded;

    // TODO: move other http_* config object into here
};

extern HttpConfig TheConfig;

} // namespace Http

#endif /* SQUID__SRC_HTTP_CONFIG_H */
