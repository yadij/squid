/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "acl/FilledChecklist.h"
#include "acl/LocalPort.h"

int
ACLLocalPortStrategy::match(Acl::Data<MatchType> * &data, ACLFilledChecklist *checklist)
{
    return data->match (checklist->my_addr.port());
}

