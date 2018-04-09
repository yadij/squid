/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 28    Access Control */

#include "squid.h"
#include "acl/Checklist.h"
#include "acl/StringData.h"
#include "ConfigParser.h"
#include "Debug.h"

Acl::StringData::StringData(Acl::StringData const &old) : stringValues(old.stringValues)
{
}

void
Acl::StringData::insert(const char *value)
{
    stringValues.insert(SBuf(value));
}

bool
Acl::StringData::match(const SBuf &tf)
{
    if (stringValues.empty() || tf.isEmpty())
        return 0;

    debugs(28, 3, "checking '" << tf << "'");

    bool found = (stringValues.find(tf) != stringValues.end());
    debugs(28, 3, tf << "' " << (found ? "found" : "NOT found"));

    return found;
}

// XXX: performance regression due to SBuf(char*) data-copies.
bool
Acl::StringData::match(char const *toFind)
{
    return match(SBuf(toFind));
}

SBufList
Acl::StringData::dump() const
{
    SBufList sl;
    sl.insert(sl.end(), stringValues.begin(), stringValues.end());
    return sl;
}

void
Acl::StringData::parse()
{
    while (const char *t = ConfigParser::strtokFile())
        stringValues.insert(SBuf(t));
}

bool
Acl::StringData::empty() const
{
    return stringValues.empty();
}

ACLData<char const *> *
Acl::StringData::clone() const
{
    /* Splay trees don't clone yet. */
    return new Acl::StringData(*this);
}

