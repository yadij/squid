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
#include "base/CharacterSet.h"
#include "globals.h"
#include "HttpRequest.h"
#include "parser/Tokenizer.h"
#include "rfc1738.h"
#include "SquidConfig.h"
#include "SquidString.h"
#include "URL_old.h"

static const char valid_hostname_chars_u[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789-._"
    "[:]"
    ;
static const char valid_hostname_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789-."
    "[:]"
    ;

const SBuf &
AnyP::Url::Asterisk()
{
    static SBuf star("*");
    return star;
}

const SBuf &
AnyP::Url::SlashPath()
{
    static SBuf slash("/");
    return slash;
}

/// the characters which truly are valid within URI
const CharacterSet &
AnyP::Url::UriValidCharacters()
{
    /* RFC 3986 section 2:
     * "
     *   A URI is composed from a limited set of characters consisting of
     *   digits, letters, and a few graphic symbols.
     * "
     */
    static const CharacterSet UriChars =
        CharacterSet("URI-Chars","") +
        // RFC 3986 section 2.2 - reserved characters
        CharacterSet("gen-delims", ":/?#[]@") +
        CharacterSet("sub-delims", "!$&'()*+,;=") +
        // RFC 3986 section 2.3 - unreserved characters
        CharacterSet::ALPHA +
        CharacterSet::DIGIT +
        CharacterSet("unreserved", "-._~") +
        // RFC 3986 section 2.1 - percent encoding "%" HEXDIG
        CharacterSet("pct-encoded", "%") +
        CharacterSet::HEXDIG;

    return UriChars;
}

void
AnyP::Url::host(const char *src)
{
    hostAddr_.setEmpty();
    hostAddr_ = src;
    if (hostAddr_.isAnyAddr()) {
        xstrncpy(host_, src, sizeof(host_));
        hostIsNumeric_ = false;
    } else {
        hostAddr_.toHostStr(host_, sizeof(host_));
        debugs(23, 3, "given IP: " << hostAddr_);
        hostIsNumeric_ = 1;
    }
    touch();
}

const SBuf &
AnyP::Url::path() const
{
    // RFC 3986 section 3.3 says path can be empty (path-abempty).
    // RFC 7230 sections 2.7.3, 5.3.1, 5.7.2 - says path cannot be empty, default to "/"
    // at least when sending and using. We must still accept path-abempty as input.
    if (path_.isEmpty() && (scheme_ == AnyP::PROTO_HTTP || scheme_ == AnyP::PROTO_HTTPS))
        return SlashPath();

    return path_;
}

/// Parse the scheme name from string b, into protocol type.
static AnyP::ProtocolType
urlParseProtocol(const SBuf &b)
{
    /* test common stuff first */
    if (b.caseCmp("http") == 0)
        return AnyP::PROTO_HTTP;

    if (b.caseCmp("ftp") == 0)
        return AnyP::PROTO_FTP;

    if (b.caseCmp("https") == 0)
        return AnyP::PROTO_HTTPS;

    if (b.caseCmp("file") == 0)
        return AnyP::PROTO_FTP;

    if (b.caseCmp("coap") == 0)
        return AnyP::PROTO_COAP;

    if (b.caseCmp("coaps") == 0)
        return AnyP::PROTO_COAPS;

    if (b.caseCmp("gopher") == 0)
        return AnyP::PROTO_GOPHER;

    if (b.caseCmp("wais") == 0)
        return AnyP::PROTO_WAIS;

    if (b.caseCmp("cache_object") == 0)
        return AnyP::PROTO_CACHE_OBJECT;

    if (b.caseCmp("urn") == 0)
        return AnyP::PROTO_URN;

    if (b.caseCmp("whois") == 0)
        return AnyP::PROTO_WHOIS;

    if (b.length() > 0)
        return AnyP::PROTO_UNKNOWN;

    return AnyP::PROTO_NONE;
}

/**
 * RFC 3986 section 3.1:
 *
 * scheme      = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
 */
bool
AnyP::Url::parseScheme(Parser::Tokenizer &tok)
{
    SBuf checkpoint = tok.remaining();

    static const CharacterSet SchemeChars = (CharacterSet("scheme", "+-.") + CharacterSet::ALPHA + CharacterSet::DIGIT);

    SBuf foundScheme;
    if (tok.prefix(foundScheme, SchemeChars) && tok.skip(':')) {
        SBuf protoStr(foundScheme);
        protoStr.toLower();
        const AnyP::ProtocolType protocol = urlParseProtocol(protoStr);
        // XXX: Performance regression. c_str() here usually reallocates
        const char *t = foundScheme.c_str();
        setScheme(protocol, t);
        return true;
    }

    tok.reset(checkpoint); // leave tok as it was when we started
    return false;
}

/*
 * This routine parses a URL. Its assumed that the URL is complete -
 * ie, the end of the string is the end of the URL. Don't pass a partial
 * URL here as this routine doesn't have any way of knowing whether
 * its partial or not (ie, it handles the case of no trailing slash as
 * being "end of host with implied path of /".
 */
bool
AnyP::Url::parse(const HttpRequestMethod& method, const char *url)
{
    LOCAL_ARRAY(char, login, MAX_URL);
    LOCAL_ARRAY(char, foundHost, MAX_URL);
    LOCAL_ARRAY(char, urlpath, MAX_URL);
    char *t = NULL;
    char *q = NULL;
    int foundPort;
    int l;
    int i;
    const char *src;
    char *dst;
    foundHost[0] = urlpath[0] = login[0] = '\0';

    if ((l = strlen(url)) + Config.appendDomainLen > (MAX_URL - 1)) {
        debugs(23, DBG_IMPORTANT, MYNAME << "URL too large (" << l << " bytes)");
        return false;
    }
    if (method == Http::METHOD_CONNECT) {
        /*
         * RFC 7230 section 5.3.3:  authority-form = authority
         *  "excluding any userinfo and its "@" delimiter"
         *
         * RFC 3986 section 3.2:    authority = [ userinfo "@" ] host [ ":" port ]
         *
         * As an HTTP(S) proxy we assume HTTPS (443) if no port provided.
         */
        foundPort = 443;

        if (sscanf(url, "[%[^]]]:%d", foundHost, &foundPort) < 1)
            if (sscanf(url, "%[^:]:%d", foundHost, &foundPort) < 1)
                return false;

    } else if ((method == Http::METHOD_OPTIONS || method == Http::METHOD_TRACE) &&
               AnyP::Url::Asterisk().cmp(url) == 0) {
        setScheme(AnyP::PROTO_HTTP, nullptr);
        parseFinish(url, foundHost, SBuf(), 80 /* HTTP default port */);
        return true;
    } else {
        /* Parse the URL: */

        // XXX: performance regression SBuf() reallocates
        SBuf tmp(url);
        Parser::Tokenizer tok(tmp);

        if (parseScheme(tok)) {

            if (getScheme() == AnyP::PROTO_URN) {
                debugs(23, 3, "Split URI '" << url << "' into proto='urn', path='" << tok.remaining() << "'");
                debugs(50, 5, "urn=" << tok.remaining());
                path(url + 4);
                return true;
            }

            foundPort = getScheme().defaultPort();
        }

        /* Then its '//' */
        static const SBuf authorityPrefix("//");
        if (!tok.skip(authorityPrefix))
            return false;

        // old parse on the rest of the URL string
        i = tok.parsedSize();
        src = url + i;

        /* Then everything until first /; thats host (and port; which we'll look for here later) */
        // bug 1881: If we don't get a "/" then we imply it was there
        // bug 3074: We could just be given a "?" or "#". These also imply "/"
        // bug 3233: whitespace is also a hostname delimiter.
        for (dst = foundHost; i < l && *src != '/' && *src != '?' && *src != '#' && *src != '\0' && !xisspace(*src); ++i, ++src, ++dst) {
            *dst = *src;
        }

        /*
         * We can't check for "i >= l" here because we could be at the end of the line
         * and have a perfectly valid URL w/ no trailing '/'. In this case we assume we've
         * been -given- a valid URL and the path is just '/'.
         */
        if (i > l)
            return false;
        *dst = '\0';

        // bug 3074: received 'path' starting with '?', '#', or '\0' implies '/'
        if (*src == '?' || *src == '#' || *src == '\0') {
            urlpath[0] = '/';
            dst = &urlpath[1];
        } else {
            dst = urlpath;
        }
        /* Then everything from / (inclusive) until \r\n or \0 - thats urlpath */
        for (; i < l && *src != '\r' && *src != '\n' && *src != '\0'; ++i, ++src, ++dst) {
            *dst = *src;
        }

        /* We -could- be at the end of the buffer here */
        if (i > l)
            return false;
        /* If the URL path is empty we set it to be "/" */
        if (dst == urlpath) {
            *dst = '/';
            ++dst;
        }
        *dst = '\0';

        /* Is there any login information? (we should eventually parse it above) */
        t = strrchr(foundHost, '@');
        if (t != NULL) {
            strncpy((char *) login, (char *) foundHost, sizeof(login)-1);
            login[sizeof(login)-1] = '\0';
            t = strrchr(login, '@');
            *t = 0;
            strncpy((char *) foundHost, t + 1, sizeof(foundHost)-1);
            foundHost[sizeof(foundHost)-1] = '\0';
            // Bug 4498: URL-unescape the login info after extraction
            rfc1738_unescape(login);
        }

        /* Is there any host information? (we should eventually parse it above) */
        if (*foundHost == '[') {
            /* strip any IPA brackets. valid under IPv6. */
            dst = foundHost;
            /* only for IPv6 sadly, pre-IPv6/URL code can't handle the clean result properly anyway. */
            src = foundHost;
            ++src;
            l = strlen(foundHost);
            i = 1;
            for (; i < l && *src != ']' && *src != '\0'; ++i, ++src, ++dst) {
                *dst = *src;
            }

            /* we moved in-place, so truncate the actual hostname found */
            *dst = '\0';
            ++dst;

            /* skip ahead to either start of port, or original EOS */
            while (*dst != '\0' && *dst != ':')
                ++dst;
            t = dst;
        } else {
            t = strrchr(foundHost, ':');

            if (t != strchr(foundHost,':') ) {
                /* RFC 2732 states IPv6 "SHOULD" be bracketed. allowing for times when its not. */
                /* RFC 3986 'update' simply modifies this to an "is" with no emphasis at all! */
                /* therefore we MUST accept the case where they are not bracketed at all. */
                t = NULL;
            }
        }

        // Bug 3183 sanity check: If scheme is present, host must be too.
        if (getScheme() != AnyP::PROTO_NONE && foundHost[0] == '\0') {
            debugs(23, DBG_IMPORTANT, "SECURITY ALERT: Missing hostname in URL '" << url << "'. see access.log for details.");
            return false;
        }

        if (t && *t == ':') {
            *t = '\0';
            ++t;
            foundPort = atoi(t);
        }
    }

    for (t = foundHost; *t; ++t)
        *t = xtolower(*t);

    if (stringHasWhitespace(foundHost)) {
        if (URI_WHITESPACE_STRIP == Config.uri_whitespace) {
            t = q = foundHost;
            while (*t) {
                if (!xisspace(*t)) {
                    *q = *t;
                    ++q;
                }
                ++t;
            }
            *q = '\0';
        }
    }

    debugs(23, 3, "Split URL '" << url << "' into proto='" << getScheme().image() << "', host='" << foundHost << "', port='" << foundPort << "', path='" << urlpath << "'");

    if (Config.onoff.check_hostnames &&
            strspn(foundHost, Config.onoff.allow_underscore ? valid_hostname_chars_u : valid_hostname_chars) != strlen(foundHost)) {
        debugs(23, DBG_IMPORTANT, MYNAME << "Illegal character in hostname '" << foundHost << "'");
        return false;
    }

    /* For IPV6 addresses also check for a colon */
    if (Config.appendDomain && !strchr(foundHost, '.') && !strchr(foundHost, ':'))
        strncat(foundHost, Config.appendDomain, SQUIDHOSTNAMELEN - strlen(foundHost) - 1);

    /* remove trailing dots from hostnames */
    while ((l = strlen(foundHost)) > 0 && foundHost[--l] == '.')
        foundHost[l] = '\0';

    /* reject duplicate or leading dots */
    if (strstr(foundHost, "..") || *foundHost == '.') {
        debugs(23, DBG_IMPORTANT, MYNAME << "Illegal hostname '" << foundHost << "'");
        return false;
    }

    if (foundPort < 1 || foundPort > 65535) {
        debugs(23, 3, "Invalid port '" << foundPort << "'");
        return false;
    }

#if HARDCODE_DENY_PORTS
    /* These ports are filtered in the default squid.conf, but
     * maybe someone wants them hardcoded... */
    if (foundPort == 7 || foundPort == 9 || foundPort == 19) {
        debugs(23, DBG_CRITICAL, MYNAME << "Deny access to port " << foundPort);
        return false;
    }
#endif

    if (stringHasWhitespace(urlpath)) {
        debugs(23, 2, "URI has whitespace: {" << url << "}");

        switch (Config.uri_whitespace) {

        case URI_WHITESPACE_DENY:
            return false;

        case URI_WHITESPACE_ALLOW:
            break;

        case URI_WHITESPACE_ENCODE:
            t = rfc1738_escape_unescaped(urlpath);
            xstrncpy(urlpath, t, MAX_URL);
            break;

        case URI_WHITESPACE_CHOP:
            *(urlpath + strcspn(urlpath, w_space)) = '\0';
            break;

        case URI_WHITESPACE_STRIP:
        default:
            t = q = urlpath;
            while (*t) {
                if (!xisspace(*t)) {
                    *q = *t;
                    ++q;
                }
                ++t;
            }
            *q = '\0';
        }
    }

    parseFinish(urlpath, foundHost, SBuf(login), foundPort);
    return true;
}

/// Update the URL object with parsed URI data.
void
AnyP::Url::parseFinish(const char *const aUrlPath,
                 const char *const aHost,
                 const SBuf &aLogin,
                 const int aPort)
{
    path(aUrlPath);
    host(aHost);
    userInfo(aLogin);
    port(aPort);
}

void
AnyP::Url::touch()
{
    absolute_.clear();
    authorityHttp_.clear();
    authorityWithPort_.clear();
}

SBuf &
AnyP::Url::authority(bool requirePort) const
{
    if (authorityHttp_.isEmpty()) {

        // both formats contain Host/IP
        authorityWithPort_.append(host());
        authorityHttp_ = authorityWithPort_;

        // authorityForm_ only has :port if it is non-default
        authorityWithPort_.appendf(":%u",port());
        if (port() != getScheme().defaultPort())
            authorityHttp_ = authorityWithPort_;
    }

    return requirePort ? authorityWithPort_ : authorityHttp_;
}

SBuf &
AnyP::Url::absolute() const
{
    if (absolute_.isEmpty()) {
        // TODO: most URL will be much shorter, avoid allocating this much
        absolute_.reserveCapacity(MAX_URL);

        absolute_.append(getScheme().image());
        absolute_.append(":",1);
        if (getScheme() != AnyP::PROTO_URN) {
            absolute_.append("//", 2);
            const bool omitUserInfo = getScheme() == AnyP::PROTO_HTTP ||
                                      getScheme() != AnyP::PROTO_HTTPS ||
                                      userInfo().isEmpty();
            if (!omitUserInfo) {
                absolute_.append(userInfo());
                absolute_.append("@", 1);
            }
            absolute_.append(authority());
        }
        absolute_.append(path());
    }

    return absolute_;
}
