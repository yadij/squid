/*
 * Copyright (C) 1996-2021 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLSTRATEGISED_H
#define SQUID_ACLSTRATEGISED_H

#include "acl/Acl.h"
#include "acl/Data.h"
#include "acl/FilledChecklist.h"
#include "acl/Strategy.h"

// XXX: Replace with a much simpler abstract ACL child class without the
// ACLStrategy parameter (and associated call forwarding). Duplicating key
// portions of the ACL class API in ACLStrategy is not needed because
// ACLStrategy is unused outside the ACLStrategised context. Existing classes
// like ACLExtUser, ACLProxyAuth, and ACLIdent seem to confirm this assertion.
// It also requires forwarding ACL info to ACLStrategy as method parameters.

/// Splits the ACL API into two individually configurable components:
/// * ACLStrategy that usually extracts information from the current transaction
/// * ACLData that usually matches information against admin-configured values
template <class MatchType>
class ACLStrategised : public ACL
{
    MEMPROXY_CLASS(ACLStrategised);

public:
    ACLStrategised(ACLData<MatchType> *, ACLStrategy<MatchType> *, char const *);
    ACLStrategised(ACLStrategised const &&) = delete;
    virtual ~ACLStrategised();

    virtual int match(MatchType const &toFind) { return data->match(toFind); }

    /* ACL API */
    virtual const Acl::Options &options() override { return matcher->options(); }
    virtual void parseFlags() override;
    virtual void parse() override { data->parse(); }
    virtual char const *typeString() const override { return type_; }
    virtual SBufList dump() const override { return data->dump(); }
    virtual bool empty() const override { return data->empty(); }
    virtual bool valid() const override { return data->valid(); }
    virtual void prepareForUse() override { data->prepareForUse(); }
    virtual int match(ACLChecklist *) override;
    virtual bool requiresAle() const { return matcher->requiresAle(); }
    virtual bool requiresRequest() const { return matcher->requiresRequest(); }
    virtual bool requiresReply() const { return matcher->requiresReply(); }

private:
    ACLData<MatchType> *data = nullptr;
    char const *type_ = nullptr;
    ACLStrategy<MatchType> *matcher = nullptr;
};

/* implementation follows */

template <class MatchType>
ACLStrategised<MatchType>::~ACLStrategised()
{
    delete data;
    delete matcher;
}

template <class MatchType>
ACLStrategised<MatchType>::ACLStrategised(ACLData<MatchType> *newData, ACLStrategy<MatchType> *theStrategy, char const *theType) :
    data(newData),
    type_(theType),
    matcher(theStrategy)
{}

template <class MatchType>
void
ACLStrategised<MatchType>::parseFlags()
{
    ParseFlags(options(), data->supportedFlags());
}

template <class MatchType>
int
ACLStrategised<MatchType>::match(ACLChecklist *cl)
{
    auto *checklist = dynamic_cast<ACLFilledChecklist*>(cl);
    assert(checklist);
    return matcher->match(data, checklist);
}

#endif /* SQUID_ACLSTRATEGISED_H */

