/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef _SQUID_SRC_SECURITY_BUMPMODE_H
#define _SQUID_SRC_SECURITY_BUMPMODE_H

namespace Security
{

/// Supported ssl-bump modes
enum BumpMode {
    bumpNone = 0,
    bumpClientFirst,
    bumpServerFirst,
    bumpPeek,
    bumpStare,
    bumpBump,
    bumpSplice,
    bumpTerminate,
    /*bumpErr,*/
    bumpEnd
};

/// Supported ssl-bump handshake steps
enum BumpStep {
    bumpStep1 = 1,
    bumpStep2,
    bumpStep3
};

/// \returns the short name of the ssl-bump mode "bm"
const char *BumpModeName(int bm);

} // namespace Security

#endif /* _SQUID_SRC_SECURITY_BUMPMODE_H */

