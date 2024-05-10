/*
 * Copyright (C) 1996-2023 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "acl/Checklist.h"
#include "acl/SslErrorData.h"
#include "security/CertError.h"
#include "security/ErrorDetail.h"

bool
ACLSslErrorData::match(const Security::CertErrors *toFind)
{
    for (const auto *err = toFind; err; err = err->next) {
        if (values.count(err->element.code))
            return true;
    }
    return false;
}

SBufList
ACLSslErrorData::dump() const
{
    SBufList sl;
    for (const auto &e : values) {
        sl.push_back(Security::ErrorNameFromCode(e));
    }
    return sl;
}

void
ACLSslErrorData::parse()
{
    while (char *t = ConfigParser::strtokFile()) {
        Security::ParseErrorString(t, values);
    }
}

