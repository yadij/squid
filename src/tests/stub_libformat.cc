/*
 * Copyright (C) 1996-2020 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "format/Format.h"

#define STUB_API "stub_libformat.cc"
#include "tests/STUB.h"

void Format::Format::assemble(MemBuf &mb, const AccessLogEntryPointer &al, int logSequenceNumber) const STUB
void Format::Format::parse(char const*) STUB
Format::Format::Format(char const*) STUB
Format::Format::~Format() STUB

