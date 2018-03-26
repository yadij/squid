/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"

#define STUB_API "acl/libapi.la"
#include "tests/STUB.h"

#include "acl/Acl.h"
namespace Acl {
// void RegisterMaker(TypeName typeName, Maker maker);
}
void *ACL::operator new(size_t sz) { return xmalloc(sz); }
void ACL::operator delete(void *X) { xfree(X); }
void ACL::ParseAclLine(ConfigParser &, ACL **) STUB
void ACL::Initialize() STUB
ACL *ACL::FindByName(const char *) STUB_RETVAL(nullptr)
ACL::ACL() {STUB_NOP}
ACL::~ACL() {STUB_NOP}
void ACL::context(const char *, const char *) STUB
bool ACL::matches(ACLChecklist *) const STUB_RETVAL(false)
void ACL::parseFlags() STUB
bool ACL::isProxyAuth() const STUB_RETVAL(false)
bool ACL::valid() const STUB_RETVAL(false)
int ACL::cacheMatchAcl(dlink_list *, ACLChecklist *) STUB_RETVAL(0)
int ACL::matchForCache(ACLChecklist *) STUB_RETVAL(0)
SBufList ACL::dumpOptions() STUB_RETVAL(SBufList())
bool ACL::requiresAle() const STUB_RETVAL(false)
bool ACL::requiresRequest() const STUB_RETVAL(false)
bool ACL::requiresReply() const STUB_RETVAL(false)

const char *AclMatchedName = nullptr;

// #include "acl/Answer.h"
#include "acl/BoolOps.h"
namespace Acl
{
//NotNode::NotNode(ACL *) {STUB}

char const *AndNode::typeString() const STUB_RETVAL("AndNode")
ACL *AndNode::clone() const STUB_RETVAL(nullptr)
void AndNode::parse() STUB
int AndNode::doMatch(ACLChecklist *, Nodes::const_iterator) const STUB_RETVAL(0)

bool OrNode::bannedAction(ACLChecklist *, Nodes::const_iterator) const STUB_RETVAL(false)
char const *OrNode::typeString() const STUB_RETVAL("OrNode")
ACL *OrNode::clone() const STUB_RETVAL(nullptr)
void OrNode::parse() STUB
int OrNode::doMatch(ACLChecklist *, Nodes::const_iterator) const STUB_RETVAL(0)
} // namespace Acl

#include "acl/Checklist.h"
ACLChecklist::NullState *ACLChecklist::NullState::Instance() STUB_RETVAL(nullptr)
void ACLChecklist::NullState::checkForAsync(ACLChecklist *) const STUB
//ACLChecklist::ACLChecklist() {STUB}
//ACLChecklist::~ACLChecklist() {STUB}
void ACLChecklist::nonBlockingCheck(ACLCB *, void *) STUB
Acl::Answer const & ACLChecklist::fastCheck() STUB_RETREF(Acl::Answer)
Acl::Answer const & ACLChecklist::fastCheck(const Acl::TreePointer) STUB_RETREF(Acl::Answer)
bool ACLChecklist::goAsync(AsyncState *) STUB_RETVAL(false)
bool ACLChecklist::matchChild(const Acl::InnerNode *, Acl::Nodes::const_iterator, const ACL *) STUB_RETVAL(false)
void ACLChecklist::markFinished(const Acl::Answer &, const char *) STUB
bool ACLChecklist::bannedAction(const Acl::Answer &) const STUB_RETVAL(false)
void ACLChecklist::banAction(const Acl::Answer &) STUB
void ACLChecklist::resumeNonBlockingCheck(AsyncState *) STUB

#include "acl/forward.h"
void Acl::Init(void) STUB

#include "acl/InnerNode.h"
namespace Acl
{
bool InnerNode::resumeMatchingAt(ACLChecklist *, Acl::Nodes::const_iterator) const STUB_RETVAL(false)
void InnerNode::prepareForUse() STUB
bool InnerNode::empty() const STUB
SBufList InnerNode::dump() const STUB
void InnerNode::lineParse() STUB
void InnerNode::add(ACL *) STUB
int InnerNode::match(ACLChecklist *checklist) STUB_RETVAL(0)
} // namespace Acl

#include "acl/Options.h"
namespace Acl {
bool OptionNameCmp::operator()(const OptionName, const OptionName) const STUB_RETVAL(false)
void ParseFlags(const Options &, const ParameterFlags &) STUB
const Options &NoOptions() STUB_RETREF(Options)
const ParameterFlags &NoFlags() STUB_RETREF(ParameterFlags)
} // namespace Acl

std::ostream &operator <<(std::ostream &os, const Acl::Option &) STUB_RETVAL(os)
std::ostream &operator <<(std::ostream &os, const Acl::Options &) STUB_RETVAL(os)

#include "acl/Tree.h"
namespace Acl
{
//    CBDATA_CLASS(Tree);
Acl::Answer Tree::winningAction() const STUB_RETVAL(Acl::Answer())
Acl::Answer Tree::lastAction() const STUB_RETVAL(Acl::Answer())
void Tree::add(ACL *, const Acl::Answer &) STUB
void Tree::add(ACL *) STUB
bool Tree::bannedAction(ACLChecklist *, Nodes::const_iterator) const STUB_RETVAL(false)
Acl::Answer Tree::actionAt(const Nodes::size_type) const STUB_RETVAL(Acl::Answer())
} // namespace Acl

