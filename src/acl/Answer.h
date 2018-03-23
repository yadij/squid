/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID__SRC_ACL_ANSWER_H
#define SQUID__SRC_ACL_ANSWER_H

#include "acl/forward.h"

typedef enum {
    // Authorization ACL result states
    ACCESS_DENIED,
    ACCESS_ALLOWED,
    ACCESS_DUNNO,

    // Authentication ACL result states
    ACCESS_AUTH_REQUIRED,    // Missing Credentials
} aclMatchCode;

namespace Acl
{

/// ACL check answer
class Answer
{
public:
    // not explicit: allow "aclMatchCode to Answer" conversions (for now)
    Answer(const aclMatchCode aCode, int aKind = 0): code(aCode), kind(aKind) {}
    Answer() = default;

    // allows implicit casting and comparison with aclMatchCode objects
    operator aclMatchCode() const { return code; }

    /// Whether an "allow" rule matched. If in doubt, use this popular method.
    /// Also use this method to treat exceptional ACCESS_DUNNO and
    /// ACCESS_AUTH_REQUIRED outcomes as if a "deny" rule matched.
    /// See also: denied().
    bool allowed() const { return code == ACCESS_ALLOWED; }

    /// Whether a "deny" rule matched. Avoid this rarely used method.
    /// Use this method (only) to treat exceptional ACCESS_DUNNO and
    /// ACCESS_AUTH_REQUIRED outcomes as if an "allow" rule matched.
    /// See also: allowed().
    bool denied() const { return code == ACCESS_DENIED; }

    /// whether Squid is uncertain about the allowed() or denied() answer
    bool conflicted() const { return !allowed() && !denied(); }

    aclMatchCode code = ACCESS_DUNNO; ///< ACCESS_* code
    int kind = 0; ///< which custom access list verb matched
};

} // namespace Acl

inline std::ostream &
operator <<(std::ostream &o, const Acl::Answer &a)
{
    switch (a) {
    case ACCESS_DENIED:
        o << "DENIED";
        break;
    case ACCESS_ALLOWED:
        o << "ALLOWED";
        break;
    case ACCESS_DUNNO:
        o << "DUNNO";
        break;
    case ACCESS_AUTH_REQUIRED:
        o << "AUTH_REQUIRED";
        break;
    }
    return o;
}

#endif /* SQUID__SRC_ACL_ANSWER_H */
