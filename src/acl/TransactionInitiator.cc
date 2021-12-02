/*
 * Copyright (C) 1996-2021 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 28    Access Control */

#include "squid.h"
#include "acl/FilledChecklist.h"
#include "acl/TransactionInitiator.h"
#include "cache_cf.h"
#include "Debug.h"
#include "HttpRequest.h"
#include "MasterXaction.h"
#include "SquidConfig.h"

Acl::TransactionInitiator::TransactionInitiator(const char *aClass) :
    class_(aClass)
{}

ACL *
Acl::TransactionInitiator::clone() const
{
    return new Acl::TransactionInitiator(*this);
}

void
Acl::TransactionInitiator::parse()
{
    while (const char *s = ConfigParser::strtokFile()) {
        initiators_ |= XactionInitiator::ParseInitiators(s);
        cfgWords.push_back(SBuf(s));
    }
}

int
Acl::TransactionInitiator::match(ACLChecklist *checklist)
{
    ACLFilledChecklist *filled = Filled((ACLChecklist*)checklist);
    assert(filled->request);
    assert(filled->request->masterXaction);
    const XactionInitiator requestInitiator = filled->request->masterXaction->initiator;
    return requestInitiator.in(initiators_) ? 1 : 0;
}

