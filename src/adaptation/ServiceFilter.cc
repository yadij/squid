/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "AccessLogEntry.h"
#include "adaptation/ServiceFilter.h"
#include "HttpReply.h"

Adaptation::ServiceFilter::ServiceFilter(Method aMethod, VectPoint aPoint, HttpRequest *aReq, HttpReply *aRep, AccessLogEntry::Pointer const &alp):
    method(aMethod),
    point(aPoint),
    request(aReq),
    reply(aRep),
    al(alp)
{
    // a lot of code assumes that there is always a virgin request or cause
    assert(request);
}

