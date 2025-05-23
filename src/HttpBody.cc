/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "base/Packable.h"
#include "HttpBody.h"

void
HttpBody::packInto(Packable * p) const
{
    assert(p);
    if (const auto size = contentSize())
        p->append(content(), size);
}

