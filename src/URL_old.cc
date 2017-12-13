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

/*
 * Test if a URL is relative.
 *
 * RFC 2396, Section 5 (Page 17) implies that in a relative URL, a '/' will
 * appear before a ':'.
 */
bool
urlIsRelative(const char *url)
{
    const char *p;

    if (url == NULL) {
        return (false);
    }
    if (*url == '\0') {
        return (false);
    }

    for (p = url; *p != '\0' && *p != ':' && *p != '/'; ++p);

    if (*p == ':') {
        return (false);
    }
    return (true);
}

/*
 * Convert a relative URL to an absolute URL using the context of a given
 * request.
 *
 * It is assumed that you have already ensured that the URL is relative.
 *
 * If NULL is returned it is an indication that the method in use in the
 * request does not distinguish between relative and absolute and you should
 * use the url unchanged.
 *
 * If non-NULL is returned, it is up to the caller to free the resulting
 * memory using safe_free().
 */
char *
urlMakeAbsolute(const HttpRequest * req, const char *relUrl)
{

    if (req->method.id() == Http::METHOD_CONNECT) {
        return (NULL);
    }

    char *urlbuf = (char *)xmalloc(MAX_URL * sizeof(char));

    if (req->url.getScheme() == AnyP::PROTO_URN) {
        // XXX: this is what the original code did, but it seems to break the
        // intended behaviour of this function. It returns the stored URN path,
        // not converting the given one into a URN...
        snprintf(urlbuf, MAX_URL, SQUIDSBUFPH, SQUIDSBUFPRINT(req->url.absolute()));
        return (urlbuf);
    }

    SBuf authorityForm = req->url.authority(); // host[:port]
    const SBuf &scheme = req->url.getScheme().image();
    size_t urllen = snprintf(urlbuf, MAX_URL, SQUIDSBUFPH "://" SQUIDSBUFPH "%s" SQUIDSBUFPH,
                             SQUIDSBUFPRINT(scheme),
                             SQUIDSBUFPRINT(req->url.userInfo()),
                             !req->url.userInfo().isEmpty() ? "@" : "",
                             SQUIDSBUFPRINT(authorityForm));

    // if the first char is '/' assume its a relative path
    // XXX: this breaks on scheme-relative URLs,
    // but we should not see those outside ESI, and rarely there.
    // XXX: also breaks on any URL containing a '/' in the query-string portion
    if (relUrl[0] == '/') {
        xstrncpy(&urlbuf[urllen], relUrl, MAX_URL - urllen - 1);
    } else {
        SBuf path = req->url.path();
        SBuf::size_type lastSlashPos = path.rfind('/');

        if (lastSlashPos == SBuf::npos) {
            // replace the whole path with the given bit(s)
            urlbuf[urllen] = '/';
            ++urllen;
            xstrncpy(&urlbuf[urllen], relUrl, MAX_URL - urllen - 1);
        } else {
            // replace only the last (file?) segment with the given bit(s)
            ++lastSlashPos;
            if (lastSlashPos > MAX_URL - urllen - 1) {
                // XXX: crops bits in the middle of the combined URL.
                lastSlashPos = MAX_URL - urllen - 1;
            }
            SBufToCstring(&urlbuf[urllen], path.substr(0,lastSlashPos));
            urllen += lastSlashPos;
            if (urllen + 1 < MAX_URL) {
                xstrncpy(&urlbuf[urllen], relUrl, MAX_URL - urllen - 1);
            }
        }
    }

    return (urlbuf);
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

/*
 * return true if we can serve requests for this method.
 */
int
urlCheckRequest(const HttpRequest * r)
{
    int rc = 0;
    /* protocol "independent" methods
     *
     * actually these methods are specific to HTTP:
     * they are methods we recieve on our HTTP port,
     * and if we had a FTP listener would not be relevant
     * there.
     *
     * So, we should delegate them to HTTP. The problem is that we
     * do not have a default protocol from the client side of HTTP.
     */

    if (r->method == Http::METHOD_CONNECT)
        return 1;

    // we support OPTIONS and TRACE directed at us (with a 501 reply, for now)
    // we also support forwarding OPTIONS and TRACE, except for the *-URI ones
    if (r->method == Http::METHOD_OPTIONS || r->method == Http::METHOD_TRACE)
        return (r->header.getInt64(Http::HdrType::MAX_FORWARDS) == 0 || r->url.path() != AnyP::Url::Asterisk());

    if (r->method == Http::METHOD_PURGE)
        return 1;

    /* does method match the protocol? */
    switch (r->url.getScheme()) {

    case AnyP::PROTO_URN:

    case AnyP::PROTO_HTTP:

    case AnyP::PROTO_CACHE_OBJECT:
        rc = 1;
        break;

    case AnyP::PROTO_FTP:

        if (r->method == Http::METHOD_PUT)
            rc = 1;

    case AnyP::PROTO_GOPHER:

    case AnyP::PROTO_WAIS:

    case AnyP::PROTO_WHOIS:
        if (r->method == Http::METHOD_GET)
            rc = 1;
        else if (r->method == Http::METHOD_HEAD)
            rc = 1;

        break;

    case AnyP::PROTO_HTTPS:
#if USE_OPENSSL
        rc = 1;
#elif USE_GNUTLS
        rc = 1;
#else
        /*
        * Squid can't originate an SSL connection, so it should
        * never receive an "https:" URL.  It should always be
        * CONNECT instead.
        */
        rc = 0;
#endif
        break;

    default:
        break;
    }

    return rc;
}

/*
 * Quick-n-dirty host extraction from a URL.  Steps:
 *      Look for a colon
 *      Skip any '/' after the colon
 *      Copy the next SQUID_MAXHOSTNAMELEN bytes to host[]
 *      Look for an ending '/' or ':' and terminate
 *      Look for login info preceeded by '@'
 */

class URLHostName
{

public:
    char * extract(char const *url);

private:
    static char Host [SQUIDHOSTNAMELEN];
    void init(char const *);
    void findHostStart();
    void trimTrailingChars();
    void trimAuth();
    char const *hostStart;
    char const *url;
};

char *
urlHostname(const char *url)
{
    return URLHostName().extract(url);
}

char URLHostName::Host[SQUIDHOSTNAMELEN];

void
URLHostName::init(char const *aUrl)
{
    Host[0] = '\0';
    url = aUrl;
}

void
URLHostName::findHostStart()
{
    if (NULL == (hostStart = strchr(url, ':')))
        return;

    ++hostStart;

    while (*hostStart != '\0' && *hostStart == '/')
        ++hostStart;

    if (*hostStart == ']')
        ++hostStart;
}

void
URLHostName::trimTrailingChars()
{
    char *t;

    if ((t = strchr(Host, '/')))
        *t = '\0';

    if ((t = strrchr(Host, ':')))
        *t = '\0';

    if ((t = strchr(Host, ']')))
        *t = '\0';
}

void
URLHostName::trimAuth()
{
    char *t;

    if ((t = strrchr(Host, '@'))) {
        ++t;
        memmove(Host, t, strlen(t) + 1);
    }
}

char *
URLHostName::extract(char const *aUrl)
{
    init(aUrl);
    findHostStart();

    if (hostStart == NULL)
        return NULL;

    xstrncpy(Host, hostStart, SQUIDHOSTNAMELEN);

    trimTrailingChars();

    trimAuth();

    return Host;
}
