/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef _SQUID_SRC_SECURITY_CERTIFICATE_H
#define _SQUID_SRC_SECURITY_CERTIFICATE_H

#include "security/forward.h"

namespace Security
{

// Wrapper functions to perform various X.509 certificate operations


/// retrieve the X.509 certificate received (if any) from the remote
/// endpoint  a TLS session.
Security::CertPointer GetPeerCertFrom(const Security::SessionPointer &);

/// API for functions called by ACLCertificateData to fetch X.509 certificate fields
typedef char const *GETX509ATTRIBUTE(const CertPointer &, const char *);

// certificate fingerprint
GETX509ATTRIBUTE GetX509Fingerprint;

// attributes of X.509 'name' fields
GETX509ATTRIBUTE GetX509CAAttribute;
GETX509ATTRIBUTE GetX509UserAttribute;

} // namespace Security

#endif /* _SQUID_SRC_SECURITY_CERTIFICATE_H */
