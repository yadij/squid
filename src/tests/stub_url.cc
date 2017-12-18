/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"

#define STUB_API "URL_old.cc"
#include "tests/STUB.h"

#include "URL_old.h"
void urlInitialize() STUB
char *urlCanonicalClean(const HttpRequest *) STUB_RETVAL(nullptr)
const char *urlCanonicalFakeHttps(const HttpRequest *) STUB_RETVAL(nullptr)
bool urlIsRelative(const char *) STUB_RETVAL(false)
char *urlMakeAbsolute(const HttpRequest *, const char *)STUB_RETVAL(nullptr)
char *urlRInternal(const char *, unsigned short, const char *, const char *) STUB_RETVAL(nullptr)
char *urlInternal(const char *, const char *) STUB_RETVAL(nullptr)
int matchDomainName(const char *, const char *, uint) STUB_RETVAL(0)
int urlCheckRequest(const HttpRequest *) STUB_RETVAL(0)
void urlExtMethodConfigure() STUB

