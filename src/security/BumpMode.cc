/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "security/BumpMode.h"

#include <vector>

const char *
Security::BumpModeName(int bm)
{
    static std::vector<const char *> BumpModeStr = {
        "none",
        "client-first",
        "server-first",
        "peek",
        "stare",
        "bump",
        "splice",
        "terminate"
        /*,"err"*/
    };

    return (bm >= 0 && bm < Security::bumpEnd) ? BumpModeStr.at(bm) : nullptr;
}

