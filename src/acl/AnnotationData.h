/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLANNOTATIONDATA_H
#define SQUID_ACLANNOTATIONDATA_H

#include "AccessLogEntry.h"
#include "acl/Data.h"
#include "Notes.h"

/// \ingroup ACLAPI
class ACLAnnotationData : public Acl::Data<NotePairs::Entry *>
{
    MEMPROXY_CLASS(ACLAnnotationData);

public:
    ACLAnnotationData();

    /// Stores annotations into pairs.
    void annotate(NotePairs::Pointer pairs, const CharacterSet *delimiters, const AccessLogEntry::Pointer &al);

    /* Acl::Data<T> API */
    virtual bool match(NotePairs::Entry *) override { return true; }
    virtual SBufList dump() const override;
    virtual void parse() override;
    virtual Acl::Data<NotePairs::Entry *> *clone() const override;
    virtual bool empty() const override { return notes->empty(); }

private:
    Notes::Pointer notes;
};

#endif /* SQUID_ACLANNOTATIONDATA_H */

