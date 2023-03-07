/*
 * Copyright (C) 1996-2023 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ESIEXPATPARSER_H
#define SQUID_ESIEXPATPARSER_H

#if USE_SQUID_ESI && HAVE_LIBEXPAT

#include "esi/Parser.h"

#if __has_include(<expat.h>)
#include <expat.h>
#endif

class ESIExpatParser : public ESIParser
{

public:
    ESIExpatParser(ESIParserClient *);
    ~ESIExpatParser() override;

    /** \retval true    on success */
    bool parse(char const *dataToParse, size_t const lengthOfData, bool const endOfStream) override;

    long int lineNumber() const override;
    char const * errorString() const override;

    EsiParserDeclaration;

private:
    /** our parser */
    mutable XML_Parser p;
    static void Start(void *data, const XML_Char *el, const char **attr);
    static void End(void *data, const XML_Char *el);
    static void Default (void *data, const XML_Char *s, int len);
    static void Comment (void *data, const XML_Char *s);
    XML_Parser &myParser() const {return p;}

    ESIParserClient *theClient;
};

#endif /* USE_SQUID_ESI */

#endif /* SQUID_ESIEXPATPARSER_H */

