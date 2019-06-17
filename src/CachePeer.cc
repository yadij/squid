/*
 * Copyright (C) 1996-2019 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "acl/Gadgets.h"
#include "CachePeer.h"
#include "defines.h"
#include "NeighborTypeDomainList.h"
#include "pconn.h"
#include "PeerPoolMgr.h"

CBDATA_CLASS_INIT(CachePeer);

CachePeer::~CachePeer()
{
    xfree(name);
    xfree(host);

    while (NeighborTypeDomainList *l = typelist) {
        typelist = l->next;
        xfree(l->domain);
        xfree(l);
    }

    aclDestroyAccessList(&access);

#if USE_CACHE_DIGESTS
    cbdataReferenceDone(digest);
    xfree(digest_url);
#endif

    delete next;

    xfree(login);

    delete standby.pool;

    // the mgr job will notice that its owner is gone and stop
    PeerPoolMgr::Checkpoint(standby.mgr, "peer gone");

    xfree(domain);
}

const char *
CachePeer::loginCredentials() const
{
   if (p->login &&
       p->login[0] != '*' &&
       strcmp(p->login, "PASS") != 0 &&
       strcmp(p->login, "PASSTHRU") != 0 &&
       strncmp(p->login, "NEGOTIATE",9) != 0 &&
       strcmp(p->login, "PROXYPASS") != 0) {

        return login;
    }

    return nullptr;
}
