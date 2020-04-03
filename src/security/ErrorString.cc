/*
 * Copyright (C) 1996-2020 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "sbuf/SBuf.h"
#include "security/ErrorString.h"

const SBuf
Security::ErrorString(const ErrorCode code)
{
#if USE_OPENSSL
    SBuf result;
    result.append(ERR_error_string(code, nullptr));
    while (const auto err = ERR_get_error()) {
        result.append("\n    ", 5);
        result.append(ERR_error_string(err, nullptr));
    }
    return result;
#elif USE_GNUTLS
    return SBuf(gnutls_strerror(code));
#else
    static const SBuf result("[no TLS library]");
    return result;
#endif
}
