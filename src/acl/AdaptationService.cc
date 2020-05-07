/*
 * Copyright (C) 1996-2020 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "acl/AdaptationService.h"
#include "acl/FilledChecklist.h"
#include "acl/IntRange.h"
#include "adaptation/Config.h"
#include "adaptation/History.h"
#include "HttpRequest.h"

int
ACLAdaptationServiceStrategy::match (ACLData<MatchType> * &data, ACLFilledChecklist *checklist)
{
    if (const auto &request = checklist->al->request) {
        if (const auto &history = request->adaptHistory()) {
            for (auto &service : history->theAdaptationServices) {
                if (data->match(service.c_str())) {
                    return 1;
                }
            }
        }
    }

    return 0;
}

