/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACLSTRATEGISED_H
#define SQUID_ACLSTRATEGISED_H

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
/// * Acl::Data that usually matches information against admin-configured values
template <class M>
class ACLStrategised : public Acl::MatchNode
{
    MEMPROXY_CLASS(ACLStrategised);

public:
    typedef M MatchType;

    ACLStrategised(Acl::Data<MatchType> *, ACLStrategy<MatchType> *, char const *);
    ACLStrategised(ACLStrategised const &&) = delete;
    ~ACLStrategised();

    virtual int match(M const &);

    /* Acl::MatchNode API */
    virtual const Acl::Options &options() override { return matcher->options(); }
    virtual void parseFlags() override;
    virtual void parse() override { data->parse(); }
    virtual char const *typeString() const override { return type_; }
    virtual SBufList dump() const override { return data->dump(); }
    virtual bool empty() const override { return data->empty(); }
    virtual bool valid() const override { return matcher->valid(); }
    virtual void prepareForUse() override { data->prepareForUse(); }
    virtual int match(ACLChecklist *) override;
    virtual bool requiresRequest() const override { return matcher->requiresRequest(); }
    virtual bool requiresReply() const override { return matcher->requiresReply(); }

private:
    Acl::Data<MatchType> *data;
    char const *type_;
    ACLStrategy<MatchType> *matcher;
};

/* implementation follows */

template <class MatchType>
ACLStrategised<MatchType>::~ACLStrategised()
{
    delete data;
}

template <class MatchType>
ACLStrategised<MatchType>::ACLStrategised(Acl::Data<MatchType> *newData, ACLStrategy<MatchType> *theStrategy, char const *theType):
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
    ACLFilledChecklist *checklist = dynamic_cast<ACLFilledChecklist*>(cl);
    assert(checklist);
    return matcher->match(data, checklist);
}

template <class MatchType>
int
ACLStrategised<MatchType>::match(MatchType const &toFind)
{
    return data->match(toFind);
}

#endif /* SQUID_ACLSTRATEGISED_H */

