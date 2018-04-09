/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLHASCOMPONENTDATA_H
#define SQUID_ACLHASCOMPONENTDATA_H

#include "acl/Checklist.h"
#include "acl/Data.h"

/// \ingroup ACLAPI
class ACLHasComponentData : public Acl::Data<ACLChecklist *>
{
    MEMPROXY_CLASS(ACLHasComponentData);

public:
    ACLHasComponentData();

    /* Acl::Data<T> API */
    virtual bool match(ACLChecklist *) override;
    virtual SBufList dump() const override;
    virtual void parse() override;
    virtual Acl::Data<ACLChecklist *> *clone() const override;
    virtual bool empty() const override { return false; }

private:
    enum ComponentKind { coRequest = 0, coResponse, coAle, coEnd };
    void parseComponent(const char *token);

    static const SBuf RequestStr;
    static const SBuf ResponseStr;
    static const SBuf AleStr;

    typedef bool (ACLChecklist::*ComponentCheck)() const;
    /// component check callbacks, ordered by component kind ID
    std::vector<ComponentCheck> componentMethods;
};

#endif

