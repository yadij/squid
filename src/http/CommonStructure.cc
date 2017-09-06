/*
 * Copyright (C) 1996-2017 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 90    HTTP Header Common Structure */

#include "squid.h"
#include "base/PackableStream.h"
#include "http/CommonStructure.h"
#include "http/one/Parser.h"
#include "http/one/Tokenizer.h"

/* parses a buffer containing field-value(s) */
bool
Http::CommonStructure::parse(const SBuf &buf, const char fvDelim, const char ivDelim)
{
    if (buf.isEmpty())
        return true; // empty field-value is valid

    Http1::Tokenizer tok(buf);
    auto whitespace = Http1::Parser::WhitespaceCharacters();

    // syntax:  header = *( OWS field-value OWS [ fvDelim ] )
    while (!tok.atEnd()) {
        (void)tok.skipAll(whitespace); // OWS

        if (tok.skip(fvDelim)) // ignore empty field-values
            continue;

        Http::CommonStructure::fvEntry temp;

        while (!tok.atEnd()) {
            (void)tok.skipAll(whitespace); // OWS

            // syntax: identifier = token [ "/" token ]
            static const CharacterSet idCharacters = CharacterSet("identifier","/") + CharacterSet::TCHAR;
            SBuf identifier, value;
            bool valueIsQuotedString = false;
            if (tok.prefix(identifier, idCharacters)) {
                (void)tok.skipAll(whitespace); // BWS

                // syntax:  [ '=' value ]
                //   value = identifier /
                //           integer /
                //           number /
                //           ascii-string /    ([1], [2])
                //           unicode-string /  ([1], [2])
                //           blob /            (NOT SUPPORTED [2])
                //           timestamp /
                //           common-structure  (NOT SUPPORTED [3])
                //
                // NOTES:
                // [1] only supported as HTTP/1 serialized form (quoted-string)
                // [2] characterset contains ivDelim, fvDelim, and/or WSP
                // [3] Squid supports only a limited 2-level data tree.
                //
                static const CharacterSet valueCharacters = CharacterSet("value","") + idCharacters;

                if (tok.skip('=')) {
                    (void)tok.skipAll(whitespace); // BWS

                    valueIsQuotedString = tok.quotedString(value, false);
                    if (!valueIsQuotedString && !tok.prefix(value, valueCharacters)) {
                        const SBuf &rem = tok.remaining();
                        debugs(90, 2, "ERROR: invalid HTTP header syntax. " << Raw("buf", rem.rawContent(), rem.length()));
                        return false;
                    }

                    (void)tok.skipAll(whitespace); // BWS
                }
            }

            temp.emplace_back(identifier, value, valueIsQuotedString);

            if (!tok.skip(ivDelim))
                break; // should be an fvDelim next
        }

        if (temp.size() > 0)
            fields.emplace_back(temp);

        if (!tok.skip(fvDelim))
            break; // invalid chars found
    }

    return tok.atEnd(); // success requires entire buffer to be consumed.
}

void
Http::CommonStructure::packInto(Packable * p, const char fvDelim, const char ivDelim) const
{
    PackableStream out(*p);

    int fvDelimNeeded = fields.size() -1;
    for (auto &field : fields) {

        int ivDelimNeeded = field.size() -1;
        for (auto &iv : field) {
            SBuf identifier;
            SBuf value;
            bool qdtext;
            std::tie(identifier, value, qdtext) = iv;

            out << identifier;
            if (!value.isEmpty()) {
                const char *quote = (qdtext ? "\"" : "");
                out << "=" << quote << value << quote;
            }

            if ((--ivDelimNeeded) > 0)
                out << ivDelim;
        }

        if ((--fvDelimNeeded) > 0)
            out << fvDelim << " ";
    }
    out.flush();
}
