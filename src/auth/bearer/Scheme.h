/*
 * Copyright (C) 1996-2017 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_AUTH_BEARER_SCHEME_H
#define SQUID_AUTH_BEARER_SCHEME_H

#if HAVE_AUTH_MODULE_BEARER

#include "auth/Scheme.h"

namespace Auth
{
namespace Bearer
{

/// scheme instance for OAuth 2.0 Bearer
class Scheme : public Auth::Scheme
{

public:
    static Auth::Scheme::Pointer GetInstance();
    Scheme() {}
    virtual ~Scheme() {}

    /* per scheme */
    virtual char const *type() const override;
    virtual void shutdownCleanup() override;
    virtual Auth::SchemeConfig *createConfig() override;

private:
    /* Not implemented */
    Scheme(Scheme const &);
    Scheme &operator=(Scheme const &);

    static Auth::Scheme::Pointer _instance;
};

} // namespace Bearer
} // namespace Auth

#endif /* HAVE_AUTH_MODULE_BEARER */
#endif /* SQUID_AUTH_BEARER_SCHEME_H */
