/*
 * Copyright (C) 1996-2026 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_SRC_ENGINES_STOREENGINE_H
#define SQUID_SRC_ENGINES_STOREENGINE_H

#include "engines/AsyncEngine.h"
#include "store/Controller.h"

namespace Store
{

/** temporary thunk across to the unrefactored store interface */
class Engine : public AsyncEngine
{
public:
    /* AsyncEngine API */
    int checkEvents(int) override {
        Root().callback();
        return EVENT_IDLE;
    };
};

} // namespace Store

#endif /* SQUID_SRC_ENGINES_STOREENGINE_H */
