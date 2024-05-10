/*
 * Copyright (C) 1996-2023 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "errorpage.h"
#include "fatal.h"
#include "sbuf/SBuf.h"
#include "ssl/ErrorDetail.h"
#include "ssl/ErrorDetailManager.h"

#include <map>

struct SslErrorAlias {
    const char *name;
    const Security::ErrorCode *errors;
};

static const Security::ErrorCode hasExpired[] = {X509_V_ERR_CERT_HAS_EXPIRED, SSL_ERROR_NONE};
static const Security::ErrorCode notYetValid[] = {X509_V_ERR_CERT_NOT_YET_VALID, SSL_ERROR_NONE};
static const Security::ErrorCode domainMismatch[] = {SQUID_X509_V_ERR_DOMAIN_MISMATCH, SSL_ERROR_NONE};
static const Security::ErrorCode certUntrusted[] = {X509_V_ERR_INVALID_CA,
                                                    X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN,
                                                    X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE,
                                                    X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT,
                                                    X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY,
                                                    X509_V_ERR_CERT_UNTRUSTED, SSL_ERROR_NONE
                                                   };
static const Security::ErrorCode certSelfSigned[] = {X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT, SSL_ERROR_NONE};

// The list of error name shortcuts  for use with ssl_error acls.
// The keys without the "ssl::" scope prefix allow shorter error
// names within the SSL options scope. This is easier than
// carefully stripping the scope prefix in Ssl::ParseErrorString().
static SslErrorAlias TheSslErrorShortcutsArray[] = {
    {"ssl::certHasExpired", hasExpired},
    {"certHasExpired", hasExpired},
    {"ssl::certNotYetValid", notYetValid},
    {"certNotYetValid", notYetValid},
    {"ssl::certDomainMismatch", domainMismatch},
    {"certDomainMismatch", domainMismatch},
    {"ssl::certUntrusted", certUntrusted},
    {"certUntrusted", certUntrusted},
    {"ssl::certSelfSigned", certSelfSigned},
    {"certSelfSigned", certSelfSigned},
    {nullptr, nullptr}
};

// Use std::map to optimize search.
typedef std::map<std::string, const Security::ErrorCode *> SslErrorShortcuts;
SslErrorShortcuts TheSslErrorShortcuts;

static void loadSslErrorShortcutsMap()
{
    assert(TheSslErrorShortcuts.empty());
    for (int i = 0; TheSslErrorShortcutsArray[i].name; ++i)
        TheSslErrorShortcuts[TheSslErrorShortcutsArray[i].name] = TheSslErrorShortcutsArray[i].errors;
}

bool
Ssl::ParseErrorString(const char *name, Security::Errors &errors)
{
    assert(name);

    const auto ssl_error = Security::ErrorCodeFromName(name);
    if (ssl_error != SSL_ERROR_NONE) {
        errors.emplace(ssl_error);
        return true;
    }

    if (xisdigit(*name)) {
        const long int value = strtol(name, nullptr, 0);
        if ((SQUID_TLS_ERR_OFFSET < value && value < SQUID_TLS_ERR_END) || // custom
                (value >= 0)) { // an official error, including SSL_ERROR_NONE
            errors.emplace(value);
            return true;
        }
        fatalf("Too small or too big TLS error code '%s'", name);
    }

    if (TheSslErrorShortcuts.empty())
        loadSslErrorShortcutsMap();

    const SslErrorShortcuts::const_iterator it = TheSslErrorShortcuts.find(name);
    if (it != TheSslErrorShortcuts.end()) {
        // Should not be empty...
        assert(it->second[0] != SSL_ERROR_NONE);
        for (int i = 0; it->second[i] != SSL_ERROR_NONE; ++i) {
            errors.emplace(it->second[i]);
        }
        return true;
    }

    fatalf("Unknown TLS error name '%s'", name);
    return false; // not reached
}

std::optional<SBuf>
Ssl::GetErrorDescr(Security::ErrorCode value)
{
    if (const auto detail = ErrorDetailsManager::GetInstance().findDefaultDetail(value))
        return detail->descr;
    return std::nullopt;
}

