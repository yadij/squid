/*
 * Copyright (C) 1996-2017 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef _SQUID_SRC_AUTH_BEARER_USERREQUEST_H
#define _SQUID_SRC_AUTH_BEARER_USERREQUEST_H

#if HAVE_AUTH_MODULE_BEARER

#include "auth/UserRequest.h"

class ConnStateData;
class HttpReply;
class HttpRequest;

namespace Auth
{
namespace Bearer
{

class UserRequest : public Auth::UserRequest
{
    MEMPROXY_CLASS(Auth::Bearer::UserRequest);

public:
    UserRequest();
    /* Auth::UserRequest API */
    virtual ~UserRequest() override;
    virtual int authenticated() const override;
    virtual void authenticate(HttpRequest *, ConnStateData *, Http::HdrType) override;
    virtual Direction module_direction() override;
    virtual void startHelperLookup(HttpRequest *, AccessLogEntry::Pointer &, AUTHCB *, void *) override;
    virtual const char *credentialsStr() override;

private:
    static HLPCB HandleReply;
};

} // namespace Bearer
} // namespace Auth

#endif /* HAVE_AUTH_MODULE_BEARER */
#endif /* _SQUID_SRC_AUTH_BEARER_USERREQUEST_H */
