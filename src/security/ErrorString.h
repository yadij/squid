/*
 * Copyright (C) 1996-2020 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID__SRC_SECURITY_ERROR_H
#define SQUID__SRC_SECURITY_ERROR_H

#include "sbuf/forward.h"
#include "security/forward.h"

namespace Security
{

/// string representation of the library-specific error code
const SBuf ErrorString(const ErrorCode);

} // namespace Security

#endif /* SQUID__SRC_SECURITY_ERROR_H */
