/*
 * Copyright (C) 1996-2021 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "parser/Exceptions.h"

void
Parser::FatalWhen(bool condition, const char *message, const SourceLocation &location)
{
    if (!condition)
        return;

    SBuf msg("FATAL: ");
    msg.append(message);
    throw Parser::Error(msg, location);
}

void
Parser::ErrorWhen(bool condition, const char *message, const SourceLocation &location)
{
    if (!condition)
        return;

    SBuf msg("ERROR: ");
    msg.append(message);
    throw Parser::Error(msg, location);
}

std::ostream &
Parser::Error::print(std::ostream &os) const
{
    os << std::runtime_error::what();
    return os;
}

