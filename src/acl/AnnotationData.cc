/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "acl/Acl.h"
#include "acl/AnnotationData.h"
#include "acl/Checklist.h"
#include "cache_cf.h"
#include "ConfigParser.h"
#include "debug/Stream.h"
#include "format/Format.h"
#include "sbuf/Algorithms.h"
#include "sbuf/Stream.h"

ACLAnnotationData::ACLAnnotationData()
    : notes(new Notes("annotation_data")) {}

SBufList
ACLAnnotationData::dump() const
{
    if (notes->empty())
        return SBufList();

    SBufStream os;
    notes->printAsAnnotationAclParameters(os);
    return SBufList{os.buf()};
}

void
ACLAnnotationData::parse()
{
    notes->parseKvPair();
    if (char *t = ConfigParser::PeekAtToken()) {
        debugs(29, DBG_CRITICAL, "FATAL: Unexpected argument '" << t << "' after annotation specification");
        self_destruct();
        return;
    }
}

void
ACLAnnotationData::annotate(NotePairs::Pointer pairs, const CharacterSet *delimiters, const AccessLogEntry::Pointer &al)
{
    notes->updateNotePairs(pairs, delimiters, al);
}

