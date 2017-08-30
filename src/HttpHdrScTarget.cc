/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 90    HTTP Cache Control Header */

#include "squid.h"
#include "HttpHdrScTarget.h"
#include "StatHist.h"

http_hdr_sc_type &operator++ (http_hdr_sc_type &aHeader);

/* copies non-extant fields from new_sc to this sc */
void
Http::Hdr::ScTarget::mergeWith(const Http::Hdr::ScTarget * new_sc)
{
    assert(new_sc);
    /* Don't touch the target - this is used to get the operations for a
     * single surrogate
     */

    if (new_sc->hasNoStore())
        noStore(true);

    if (new_sc->hasNoStoreRemote())
        noStoreRemote(true);

    if (new_sc->hasMaxAge() && !hasMaxAge()) {
        maxAge(new_sc->maxAge());
        maxStale(new_sc->maxStale());
    }

    if (new_sc->hasContent() && !hasContent())
        Content(new_sc->content());

}

void
Http::Hdr::ScTarget::updateStats(StatHist * hist) const
{
    http_hdr_sc_type c;

    for (c = SC_NO_STORE; c < SC_ENUM_END; ++c)
        if (isSet(c))
            hist->count(c);
}

