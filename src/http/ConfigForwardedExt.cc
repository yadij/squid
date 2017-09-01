/*
 * Copyright (C) 1996-2017 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "base/Packable.h"
#include "ConfigParser.h"
#include "Debug.h"
#include "globals.h"
#include "http/ConfigForwardedExt.h"

void
Http::ConfigForwardedExt::parse(ConfigParser &parser)
{
    while (auto *token = parser.NextToken()) {
        if (strcmp(token, "transparent") == 0) {
            mode = Http::ConfigForwardedExt::fwdTransparent;

        } else if (strcmp(token, "delete") == 0) {
            mode = Http::ConfigForwardedExt::fwdDelete;

        } else {
            debugs(3, DBG_PARSE_NOTE(DBG_IMPORTANT), "ERROR: http_ext_forwarded unknown option '" << token << "'");
        }
    }
}

void
Http::ConfigForwardedExt::dump(Packable *p, const char *name)
{
    switch (mode) {
    case Http::ConfigForwardedExt::fwdTransparent:
        // default, nothing to do.
        return;
    case Http::ConfigForwardedExt::fwdDelete:
        p->appendf("%s delete", name);
        break;
    }

    p->append("\n", 1);
}
