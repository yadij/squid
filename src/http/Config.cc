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
Http::ExtForwardedCfg::parse(ConfigParser &parser)
{
    while (auto *token = parser.NextToken()) {
        if (strcmp(token, "transparent") == 0) {
            mode = Http::ExtForwardedCfg::fwdTransparent;

        } else if (strcmp(token, "delete") == 0) {
            mode = Http::ExtForwardedCfg::fwdDelete;

        } else {
            debugs(3, DBG_PARSE_NOTE(DBG_IMPORTANT), "ERROR: http_ext_forwarded unknown option '" << token << "'");
        }
    }
}

void
Http::ExtForwardedCfg::dump(Packable *p, const char *name)
{
    switch (mode) {
    case Http::ExtForwardedCfg::fwdTransparent:
        // default, nothing to do.
        return;
    case Http::ExtForwardedCfg::fwdDelete:
        p->appendf("%s delete", name);
        break;
    }

    p->append("\n", 1);
}

void
Http::HttpConfig::parse(const char *directiveName, ConfigParser &parser)
{
    if (strcmp(directiveName, "http_ext_forwarded") == 0) {
        extForwarded.parse(parser);
        return;
    }
}
