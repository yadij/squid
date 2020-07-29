/*
 * Copyright (C) 1996-2020 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 82    External ACL */

#include "squid.h"
#include "ExternalACLEntry.h"
#include "SquidTime.h"

/******************************************************************
 * external_acl cache
 */

ExternalACLEntry::ExternalACLEntry() :
    notes()
{
    lru.next = lru.prev = NULL;
    result = ACCESS_DENIED;
    date = 0;
    def = NULL;
}

ExternalACLEntry::~ExternalACLEntry()
{
    safe_free(key);
}

