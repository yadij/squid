/*
 * Copyright (C) 1996-2017 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 23    URL Parsing */

#include "squid.h"
#include "anyp/Url.h"
#include "globals.h"
#include "HttpRequest.h"
#include "rfc1738.h"
#include "SquidConfig.h"
#include "SquidString.h"
#include "URL_old.h"

void
urlInitialize(void)
{
    debugs(23, 5, "urlInitialize: Initializing...");
    /* this ensures that the number of protocol strings is the same as
     * the enum slots allocated because the last enum is always 'MAX'.
     */
    assert(strcmp(AnyP::ProtocolType_str[AnyP::PROTO_MAX], "MAX") == 0);
    /*
     * These test that our matchDomainName() function works the
     * way we expect it to.
     */
    assert(0 == matchDomainName("foo.com", "foo.com"));
    assert(0 == matchDomainName(".foo.com", "foo.com"));
    assert(0 == matchDomainName("foo.com", ".foo.com"));
    assert(0 == matchDomainName(".foo.com", ".foo.com"));
    assert(0 == matchDomainName("x.foo.com", ".foo.com"));
    assert(0 == matchDomainName("y.x.foo.com", ".foo.com"));
    assert(0 != matchDomainName("x.foo.com", "foo.com"));
    assert(0 != matchDomainName("foo.com", "x.foo.com"));
    assert(0 != matchDomainName("bar.com", "foo.com"));
    assert(0 != matchDomainName(".bar.com", "foo.com"));
    assert(0 != matchDomainName(".bar.com", ".foo.com"));
    assert(0 != matchDomainName("bar.com", ".foo.com"));
    assert(0 < matchDomainName("zzz.com", "foo.com"));
    assert(0 > matchDomainName("aaa.com", "foo.com"));
    assert(0 == matchDomainName("FOO.com", "foo.COM"));
    assert(0 < matchDomainName("bfoo.com", "afoo.com"));
    assert(0 > matchDomainName("afoo.com", "bfoo.com"));
    assert(0 < matchDomainName("x-foo.com", ".foo.com"));

    assert(0 == matchDomainName(".foo.com", ".foo.com", mdnRejectSubsubDomains));
    assert(0 == matchDomainName("x.foo.com", ".foo.com", mdnRejectSubsubDomains));
    assert(0 != matchDomainName("y.x.foo.com", ".foo.com", mdnRejectSubsubDomains));
    assert(0 != matchDomainName(".x.foo.com", ".foo.com", mdnRejectSubsubDomains));

    assert(0 == matchDomainName("*.foo.com", "x.foo.com", mdnHonorWildcards));
    assert(0 == matchDomainName("*.foo.com", ".x.foo.com", mdnHonorWildcards));
    assert(0 == matchDomainName("*.foo.com", ".foo.com", mdnHonorWildcards));
    assert(0 != matchDomainName("*.foo.com", "foo.com", mdnHonorWildcards));

    /* more cases? */
}

/** \todo AYJ: Performance: This is an *almost* duplicate of HttpRequest::effectiveRequestUri(). But elides the query-string.
 *        After copying it on in the first place! Would be less code to merge the two with a flag parameter.
 *        and never copy the query-string part in the first place
 */
char *
urlCanonicalClean(const HttpRequest * request)
{
    LOCAL_ARRAY(char, buf, MAX_URL);

    snprintf(buf, sizeof(buf), SQUIDSBUFPH, SQUIDSBUFPRINT(request->effectiveRequestUri()));
    buf[sizeof(buf)-1] = '\0';

    // URN, CONNECT method, and non-stripped URIs can go straight out
    if (Config.onoff.strip_query_terms && !(request->method == Http::METHOD_CONNECT || request->url.getScheme() == AnyP::PROTO_URN)) {
        // strip anything AFTER a question-mark
        // leaving the '?' in place
        if (auto t = strchr(buf, '?')) {
            *(++t) = '\0';
        }
    }

    if (stringHasCntl(buf))
        xstrncpy(buf, rfc1738_escape_unescaped(buf), MAX_URL);

    return buf;
}

/**
 * Yet another alternative to urlCanonical.
 * This one adds the https:// parts to Http::METHOD_CONNECT URL
 * for use in error page outputs.
 * Luckily we can leverage the others instead of duplicating.
 */
const char *
urlCanonicalFakeHttps(const HttpRequest * request)
{
    LOCAL_ARRAY(char, buf, MAX_URL);

    // method CONNECT and port HTTPS
    if (request->method == Http::METHOD_CONNECT && request->url.port() == 443) {
        snprintf(buf, MAX_URL, "https://%s/*", request->url.host());
        return buf;
    }

    // else do the normal complete canonical thing.
    return urlCanonicalClean(request);
}

int
matchDomainName(const char *h, const char *d, uint flags)
{
    int dl;
    int hl;

    const bool hostIncludesSubdomains = (*h == '.');
    while ('.' == *h)
        ++h;

    hl = strlen(h);

    if (hl == 0)
        return -1;

    dl = strlen(d);

    /*
     * Start at the ends of the two strings and work towards the
     * beginning.
     */
    while (xtolower(h[--hl]) == xtolower(d[--dl])) {
        if (hl == 0 && dl == 0) {
            /*
             * We made it all the way to the beginning of both
             * strings without finding any difference.
             */
            return 0;
        }

        if (0 == hl) {
            /*
             * The host string is shorter than the domain string.
             * There is only one case when this can be a match.
             * If the domain is just one character longer, and if
             * that character is a leading '.' then we call it a
             * match.
             */

            if (1 == dl && '.' == d[0])
                return 0;
            else
                return -1;
        }

        if (0 == dl) {
            /*
             * The domain string is shorter than the host string.
             * This is a match only if the first domain character
             * is a leading '.'.
             */

            if ('.' == d[0]) {
                if (flags & mdnRejectSubsubDomains) {
                    // Check for sub-sub domain and reject
                    while(--hl >= 0 && h[hl] != '.');
                    if (hl < 0) {
                        // No sub-sub domain found, but reject if there is a
                        // leading dot in given host string (which is removed
                        // before the check is started).
                        return hostIncludesSubdomains ? 1 : 0;
                    } else
                        return 1; // sub-sub domain, reject
                } else
                    return 0;
            } else
                return 1;
        }
    }

    /*
     * We found different characters in the same position (from the end).
     */

    // If the h has a form of "*.foo.com" and d has a form of "x.foo.com"
    // then the h[hl] points to '*', h[hl+1] to '.' and d[dl] to 'x'
    // The following checks are safe, the "h[hl + 1]" in the worst case is '\0'.
    if ((flags & mdnHonorWildcards) && h[hl] == '*' && h[hl + 1] == '.')
        return 0;

    /*
     * If one of those character is '.' then its special.  In order
     * for splay tree sorting to work properly, "x-foo.com" must
     * be greater than ".foo.com" even though '-' is less than '.'.
     */
    if ('.' == d[dl])
        return 1;

    if ('.' == h[hl])
        return -1;

    return (xtolower(h[hl]) - xtolower(d[dl]));
}
