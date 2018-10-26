/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 28    Access Control */

#include "squid.h"
#include "acl/Certificate.h"
#include "fde.h"
#include "security/Certificate.h"

int
ACLCertificateStrategy::match (ACLData<MatchType> * &data, ACLFilledChecklist *checklist)
{
    const int fd = checklist->fd();
    const bool goodDescriptor = 0 <= fd && fd <= Biggest_FD;
    if (!goodDescriptor)
        return false; // XXX: print a debug note?

    auto cert = Security::GetPeerCertFrom(fd_table[fd].ssl);
    return data->match(cert);
}

