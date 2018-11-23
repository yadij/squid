/*
 * Copyright (C) 1996-2020 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"

#include <cppunit/TestAssert.h>

#include "anyp/Uri.h"
#include "Debug.h"
#include "http/RequestMethod.h"
#include "SquidConfig.h"
#include "tests/testURL.h"
#include "unitTestMain.h"

#include <sstream>
#include <vector>

CPPUNIT_TEST_SUITE_REGISTRATION( testURL );

/* init memory pools */

void
testURL::setUp()
{
    Mem::Init();
    AnyP::UriScheme::Init();
}

/*
 * we can construct a URL with a AnyP::UriScheme.
 * This creates a URL for that scheme.
 */
void
testURL::testConstructScheme()
{
    AnyP::UriScheme empty_scheme;
    AnyP::Uri protoless_url(AnyP::PROTO_NONE);
    CPPUNIT_ASSERT_EQUAL(empty_scheme, protoless_url.getScheme());

    AnyP::UriScheme ftp_scheme(AnyP::PROTO_FTP);
    AnyP::Uri ftp_url(AnyP::PROTO_FTP);
    CPPUNIT_ASSERT_EQUAL(ftp_scheme, ftp_url.getScheme());
}

/*
 * a default constructed URL has scheme "NONE".
 * Also, we should be able to use new and delete on
 * scheme instances.
 */
void
testURL::testDefaultConstructor()
{
    AnyP::UriScheme aScheme;
    AnyP::Uri aUrl;
    CPPUNIT_ASSERT_EQUAL(aScheme, aUrl.getScheme());

    auto *urlPointer = new AnyP::Uri;
    CPPUNIT_ASSERT(urlPointer != NULL);
    delete urlPointer;
}

void
testURL::testCanonicalCleanWithoutRequest()
{
    const std::vector<std::pair<SBuf,SBuf>> authorityPrefix = {
        {SBuf(),SBuf()},
        {SBuf("http://example.com"),SBuf("http://example.com")},
        {SBuf("http://example.com:1234"),SBuf("http://example.com:1234")}
// XXX: path with CTL chars
// XXX: path with ASCII-extended chars
    };

    const std::vector<std::pair<SBuf,SBuf>> path = {
        {SBuf(),SBuf()},
        {SBuf("/"),SBuf("/")},
        {SBuf("/path"),SBuf("/path")}
// XXX: path with CTL chars
// XXX: path with ASCII-extended chars
   };

    const std::vector<std::pair<SBuf,SBuf>> query =  {
        {SBuf(),SBuf()},
        {SBuf("?"),SBuf("?")},
        {SBuf("?query"),SBuf("?query")}
// XXX: query with CTL chars
// XXX: query with ASCII-extended chars
    };

    const std::vector<std::pair<SBuf,SBuf>> fragment = {
        {SBuf(),SBuf()},
        {SBuf("#"),SBuf("#")},
        {SBuf("#fragment"),SBuf("#fragment")}
// XXX: fragment with CTL chars
// XXX: fragment with ASCII-extended chars
    };

    const HttpRequestMethod mNil; // METHOD_NONE is sufficient for non-CONNECT tests
    const AnyP::UriScheme sNil;   // PROTO_NONE is sufficient for non-URN tests

    for (const auto a : authorityPrefix) {
        for (const auto p : path) {
            for (const auto q : query) {
                for (const auto f : fragment) {
                    SBuf in(a.first);
                    in.append(p.first);
                    in.append(q.first);
                    in.append(f.first);

                    Config.onoff.strip_query_terms = false;
                    SBuf outA(a.second);
                    outA.append(p.second);
                    outA.append(q.second);
                    outA.append(f.second);
                    CPPUNIT_ASSERT_EQUAL(outA, urlCanonicalCleanWithoutRequest(in, mNil, sNil));

                    Config.onoff.strip_query_terms = true;
                    SBuf outB(a.second);
                    outB.append(p.second);
                    if (!q.second.isEmpty())
                        outB.append('?');
                    else if (!f.second.isEmpty())
                        outB.append('#');
                    CPPUNIT_ASSERT_EQUAL(outB, urlCanonicalCleanWithoutRequest(in, mNil, sNil));
                }
            }
        }
    }

    // TODO test CONNECT URI cleaning

    // TODO test URN cleaning
}
