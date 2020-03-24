/*
 * Copyright (C) 1996-2020 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef _SQUID__SRC_FORMAT_FORWARD_H
#define _SQUID__SRC_FORMAT_FORWARD_H

#include "base/RefCount.h"

/// logformat directive and %code's
namespace Format
{

class Format;
typedef RefCount<Format> FormatPointer;

} // namespace Format

#endif /* _SQUID__SRC_FORMAT_FORWARD_H */
