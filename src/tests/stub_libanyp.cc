/*
 * Copyright (C) 1996-2017 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"

#define STUB_API "URL_old.cc"
#include "tests/STUB.h"

#include "anyp/Url.h"
AnyP::Url::Url(AnyP::UriScheme const &) {STUB}
void AnyP::Url::touch() STUB
bool AnyP::Url::parse(const HttpRequestMethod&, const char *) STUB_RETVAL(true)
void AnyP::Url::host(const char *) STUB
static SBuf nil;
const SBuf &AnyP::Url::path() const STUB_RETVAL(nil)
const SBuf &AnyP::Url::SlashPath()
{
    static SBuf slash("/");
    return slash;
}
const SBuf &AnyP::Url::Asterisk()
{
    static SBuf asterisk("*");
    return asterisk;
}
SBuf &AnyP::Url::authority(bool) const STUB_RETVAL(nil)
SBuf &AnyP::Url::absolute() const STUB_RETVAL(nil)
