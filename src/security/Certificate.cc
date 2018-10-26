/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "Debug.h"
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

int
Security::GetCertAttributeNID(char *newAttribute)
{
#if USE_OPENSSL
    auto nid = OBJ_txt2nid(newAttribute);
    if (nid == 0) {
        const auto span = strspn(newAttribute, "0123456789.");
        if(newAttribute[span] == '\0') { // looks like a numerical OID
            // create a new object based on this attribute

            // NOTE: Not a [bad] leak: If the same attribute
            // has been added before, the OBJ_txt2nid call
            // would return a valid nid value.
            // TODO: call OBJ_cleanup() on reconfigure?
            nid = OBJ_create(newAttribute, newAttribute,  newAttribute);
            debugs(28, 7, "New X.509 Certificate attribute created with nid=" << nid << ", name: " << newAttribute);
        }
    }
    return nid;

#elif USE_GNUTLS
    // TODO: implement. for now trigger invalid NID error handling
    return 0;

#else
    return 0;
#endif
}

#if USE_OPENSSL
// TODO: rename to match coding style guidelines
static const char *
ssl_get_attribute(const char *field, X509_NAME * name, const char *attribute_name)
{
    static char buffer[1024];
    buffer[0] = '\0';

    if (strcmp(attribute_name, "DN") == 0) {
        (void)X509_NAME_oneline(name, buffer, sizeof(buffer));
    } else {
        int nid = OBJ_txt2nid(const_cast<char *>(attribute_name));
        if (nid == 0) {
            debugs(83, DBG_IMPORTANT, "WARNING: Unknown X.509 Certificate " << field << " attribute name '" << attribute_name << "'");
            return nullptr;
        }
        X509_NAME_get_text_by_NID(name, nid, buffer, sizeof(buffer));
    }

    return *buffer ? buffer : nullptr;
}
#endif

const char *
Security::GetX509CAAttribute(const Security::CertPointer &cert, const char *attribute_name)
{
    if (!cert)
        return nullptr;

#if USE_OPENSSL
    auto name = X509_get_issuer_name(cert.get());
    return ssl_get_attribute("Issuer", name, attribute_name);

#elif USE_GNUTLS
    return nullptr; // TODO: implement

#else
    return nullptr;
#endif
}

const char *
Security::GetX509Fingerprint(const Security::CertPointer &cert, const char *)
{
    if (!cert)
        return nullptr;

#if USE_OPENSSL
    unsigned int n;
    unsigned char md[EVP_MAX_MD_SIZE];
    if (!X509_digest(cert.get(), EVP_sha1(), md, &n))
        return nullptr;

    static char buf[1024];
    assert(3 * n + 1 < sizeof(buf));

    char *s = buf;
    for (unsigned int i=0; i < n; ++i, s += 3) {
        const char term = (i + 1 < n) ? ':' : '\0';
        auto x = snprintf(s, 4, "%02X%c", md[i], term);
        assert(x > 0);
    }
    return buf;

#elif USE_GNUTLS
    return nullptr; // TODO implement

#else
    return nullptr;
#endif
}

const char *
Security::GetX509UserAttribute(const Security::CertPointer &cert, const char *attribute_name)
{
    if (!cert)
        return nullptr;

#if USE_OPENSSL
    auto name = X509_get_subject_name(cert.get());
    return ssl_get_attribute("Subject", name, attribute_name);

#elif USE_GNUTLS
    return nullptr; // TODO: implement

#else
    return nullptr;
#endif
}

