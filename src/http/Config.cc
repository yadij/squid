/*
 * Copyright (C) 1996-2017 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "ConfigParser.h"
#include "Debug.h"
#include "globals.h"
#include "http/Config.h"

Http::HttpConfig Http::TheConfig;

void
Http::HttpConfig::parse(const char *directiveName, ConfigParser &parser)
{
    if (strcmp(directiveName, "http_ext_forwarded") == 0) {
        extForwarded.parse(parser);
        return;
    }
}

void
Http::HttpConfig::dump(Packable *p, const char *name)
{
    extForwarded.dump(p, name);
}
