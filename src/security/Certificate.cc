/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "security/Certificate.h"

Security::CertPointer
Security::GetPeerCertFrom(const Security::SessionPointer &s)
{
    Security::CertPointer cert;

#if USE_OPENSSL
    cert.resetWithoutLocking(SSL_get_peer_certificate(s.get()));

#elif USE_GNUTLS
    // TODO: implement. for now leaves cert as nil.

#else
    // leave cert as nil
#endif

    return cert;
}

