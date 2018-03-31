/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLNOTEDATA_H
#define SQUID_ACLNOTEDATA_H

#include "acl/Data.h"
#include "Notes.h"
#include "SquidString.h"

class ACLStringData;

/// \ingroup ACLAPI
class ACLNoteData : public ACLData<NotePairs::Entry *>
{
    MEMPROXY_CLASS(ACLNoteData);

public:
    ACLNoteData();
    virtual ~ACLNoteData();

    /* ACLData<T> API */
    virtual bool match(NotePairs::Entry *) override;
    virtual SBufList dump() const override;
    virtual void parse() override;
    virtual ACLData *clone() const override;
    virtual bool empty() const override;

private:
    SBuf name;                   ///< Note name to check. It is always set
    ACLStringData *values; ///< if set, at least one value must match
};

#endif /* SQUID_ACLNOTEDATA_H */

