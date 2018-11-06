/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "acl/Checklist.h"
#include "acl/NoteData.h"
#include "acl/StringData.h"
#include "ConfigParser.h"
#include "Debug.h"
#include "sbuf/StringConvert.h"
#include "wordlist.h"

ACLNoteData::ACLNoteData() : values(new Acl::StringData)
{}

ACLNoteData::~ACLNoteData()
{
    delete values;
}

bool
ACLNoteData::match(NotePairs::Entry *entry)
{
    if (entry->name().cmp(name) != 0)
        return false; // name mismatch

    // a name-only note ACL matches any value; others require a values match
    return values->empty() ||
           values->match(entry->value());
}

SBufList
ACLNoteData::dump() const
{
    SBufList sl;
    sl.push_back(name);
    sl.splice(sl.end(), values->dump());
    return sl;
}

void
ACLNoteData::parse()
{
    char* t = ConfigParser::strtokFile();
    assert (t != NULL);
    name = t;
    values->parse();
}

bool
ACLNoteData::empty() const
{
    return name.isEmpty();
}

Acl::Data<NotePairs::Entry *> *
ACLNoteData::clone() const
{
    ACLNoteData * result = new ACLNoteData;
    result->values = dynamic_cast<Acl::StringData*>(values->clone());
    assert(result->values);
    result->name = name;
    return result;
}

