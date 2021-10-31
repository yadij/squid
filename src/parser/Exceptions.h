/*
 * Copyright (C) 1996-2021 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID__SRC_PARSER_EXCEPTIONS_H
#define SQUID__SRC_PARSER_EXCEPTIONS_H

#include "base/TextException.h"
#include "sbuf/SBuf.h"

namespace Parser
{

/// throw "ERROR:" message only if the condition is true
static void ErrorWhen(bool condition, const char *message, const SourceLocation &);

/// throw "FATAL:" message only if the condition is true
static void FatalWhen(bool condition, const char *message, const SourceLocation &);

/// An std::runtime_error for Parser exceptions.
/// Message formatted without source details for admin view.
class Error : public TextException
{
public:
    Error(SBuf &message, const SourceLocation &location) :
        TextException(message, location)
    {}

    /* TextException API */
    virtual ~Error() throw() override;
    virtual std::ostream &print(std::ostream &) const override;
};

} // namespace Parser

#endif /* SQUID__SRC_PARSER_EXCEPTIONS_H */
