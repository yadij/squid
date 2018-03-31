/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACL_FORWARD_H
#define SQUID_ACL_FORWARD_H

#include "base/CbcPointer.h"
#include "base/RefCount.h"

class ACLChecklist;
class ACLFilledChecklist;

class AclDenyInfoList;
class AclSizeLimit;

namespace Acl
{

class Address;
class Answer;
class InnerNode;
class MatchNode;
class NotNode;
class AndNode;
class OrNode;
class Tree;
typedef CbcPointer<Acl::Tree> TreePointer;

/// prepares to parse ACLs configuration
void Init(void);

} // namespace Acl

typedef void ACLCB(Acl::Answer, void *);

#define ACL_NAME_SZ 64

// TODO: Consider renaming all users and removing. Cons: hides the difference
// between ACLList tree without actions and acl_access Tree with actions.
#define acl_accessPointer Acl::TreePointer
#define ACLListPointer Acl::TreePointer

class ExternalACLEntry;
typedef RefCount<ExternalACLEntry> ExternalACLEntryPointer;

#endif /* SQUID_ACL_FORWARD_H */

