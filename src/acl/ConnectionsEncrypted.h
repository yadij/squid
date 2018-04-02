/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ACL_CONNECTIONS_ENCRYPTED_H
#define SQUID_ACL_CONNECTIONS_ENCRYPTED_H

#include "acl/Checklist.h"
#include "acl/MatchNode.h"

namespace Acl
{

class ConnectionsEncrypted : public Acl::MatchNode
{
    MEMPROXY_CLASS(ConnectionsEncrypted);

public:
    ConnectionsEncrypted(char const *);
    ConnectionsEncrypted(ConnectionsEncrypted const &);
    virtual ~ConnectionsEncrypted();
    ConnectionsEncrypted &operator =(ConnectionsEncrypted const &);

    /* Acl::MatchNode API */
    virtual void parse() override;
    virtual char const *typeString() const override;
    virtual SBufList dump() const override;
    virtual bool empty() const override;
    virtual int match(ACLChecklist *) override;

protected:
    char const *class_;
};

} // namespace Acl

#endif /* SQUID_ACL_CONNECTIONS_ENCRYPTED_H */

