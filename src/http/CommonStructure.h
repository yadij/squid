/*
 * Copyright (C) 1996-2017 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID__SRC_HTTP_COMMONSTRUCTURE_H
#define SQUID__SRC_HTTP_COMMONSTRUCTURE_H

#include "http/forward.h"
#include "mem/forward.h"
#include "sbuf/SBuf.h"

#include <list>
#include <tuple>

class Packable;

namespace Http
{

/**
 * The tree of tokens for an HTTP header which uses 'common structure'
 * syntax defined in draft-ietf-httpbis-header-structure-01
 *
 * Such headers are represented in Squid as std::list of
 * field-values, each of which contains a std::list of
 * identifier + value std::pair.
 */
class CommonStructure
{
    MEMPROXY_CLASS(Http::CommonStructure);

public:
    CommonStructure(const CommonStructure &);
    CommonStructure() {}
    ~CommonStructure() {}

    /**
     * Parses a single Mime HTTP header field-value.
     * May be called repeatedly. Each call is treated as a new header
     * line to be appended with an implicit field-value delimiter.
     *
     * \param fvDelim the delimiter to use between field-value entries
     *                when multiple are listed in a single header line.
     * \param ivDelim the delimiter to use between identifier-value
     *                entries when multiple exist in a single field-value.
     */
    bool parse(const SBuf &, const char fvDelim, const char ivDelim);

    /**
     * Serialize this header into a single Mime HTTP header field-value.
     *
     * \param fvDelim the delimiter to use between field-value entries
     *                when multiple exist in the header data.
     * \param ivDelim the delimiter to use between identifier-value
     *                entries when multiple exist in a single field-value.
     */
    void packInto(Packable *, const char fvDelim, const char ivDelim) const;

    /// An identifier[=value] pair (value is optional),
    /// plus a flag to indicate whether value was a quoted-string.
    /// Leaving values in their string form without attempting to
    /// interpret more advanced types.
    typedef std::tuple<SBuf, SBuf, bool> ivPair;

    /// An HTTP field-value is a set of entries (maybe singular).
    typedef std::list<ivPair> fvEntry;

    /// An HTTP header is a list of field-values (maybe singular).
    std::list<fvEntry> fields;
};

} // namespace Http

#endif /* SQUID__SRC_HTTP_COMMONSTRUCTURE_H */
