/*
 * Copyright (C) 1996-2023 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 23    URL Parsing */

#include "squid.h"
#include "anyp/Uri.h"
#include "base/Raw.h"
#include "globals.h"
#include "HttpRequest.h"
#include "parser/Tokenizer.h"
#include "rfc1738.h"
#include "sbuf/Stream.h"
#include "SquidConfig.h"
#include "SquidMath.h"

/// Characters which are valid within a URI userinfo section
static const CharacterSet &
UserInfoChars()
{
    /*
     * RFC 3986 section 3.2.1
     *
     *  userinfo      = *( unreserved / pct-encoded / sub-delims / ":" )
     *  unreserved    = ALPHA / DIGIT / "-" / "." / "_" / "~"
     *  pct-encoded   = "%" HEXDIG HEXDIG
     *  sub-delims    = "!" / "$" / "&" / "'" / "(" / ")" / "*" / "+" / "," / ";" / "="
     */
    static const auto userInfoValid = CharacterSet("userinfo", ":-._~%!$&'()*+,;=") +
                                      CharacterSet::ALPHA +
                                      CharacterSet::DIGIT;
    return userInfoValid;
}

/// Characters which are valid for a domain name in the URI-authority section
/// and optionally '_' when allow_underscore is configured
static const CharacterSet &
DomainNameChars()
{
    /*
     * RFC 3986 section 3.2.2 defines an overly-broad set of valid
     * characters. Then delegates further limitations to whichever
     * specification governs the registry used to resolve the hostname.
     *
     * URLs accepted by Squid are resolved with DNS.
     *
     * RFC 1034 section 3.5
     *
     *  <subdomain> ::= <label> | <subdomain> "." <label>
     *
     *  <label> ::= <letter> [ [ <ldh-str> ] <let-dig> ]
     *
     *  <ldh-str> ::= <let-dig-hyp> | <let-dig-hyp> <ldh-str>
     *
     *  <let-dig-hyp> ::= <let-dig> | "-"
     *
     *  <let-dig> ::= <letter> | <digit>
     *
     *  <letter> ::= any one of the 52 alphabetic characters A through Z in
     *               upper case and a through z in lower case
     *
     *  <digit> ::= any one of the ten digits 0 through 9
     */
    static const auto dnsValid = CharacterSet("domain", ".-") +
                                 CharacterSet::ALPHA +
                                 CharacterSet::DIGIT;

    static const auto squidLax = CharacterSet("host_name","_") +
                                 dnsValid;

    return (Config.onoff.allow_underscore ? squidLax : dnsValid);
}

/// Characters which are valid for the URI-path section.
/// Includes delimiters which we must ignore due to Squid
/// still combining the path?query#fragment sections.
static const CharacterSet &
PathChars()
{
    /*
     * RFC 3986 section 3.3 (duplicate cases excluded)
     *
     *  path          = path-abempty    ; begins with "/" or is empty
     *  path-abempty  = *( "/" segment )
     *  segment       = *pchar
     *  pchar         = unreserved / pct-encoded / sub-delims / ":" / "@"
     *  unreserved    = ALPHA / DIGIT / "-" / "." / "_" / "~"
     *  pct-encoded   = "%" HEXDIG HEXDIG
     *  sub-delims    = "!" / "$" / "&" / "'" / "(" / ")" / "*" / "+" / "," / ";" / "="
     */
    static const auto pathValid = CharacterSet("path","/") + CharacterSet::PCHAR;

    /*
     * RFC 3986 section 3.4
     *
     *  query         = *( pchar / "/" / "?" )
     */
    static const auto queryValid = CharacterSet("query","/?") + CharacterSet::PCHAR;

    /*
     * RFC 3986 section 3.5
     *
     *  fragment      = *( pchar / "/" / "?" )
     */
    static const auto fragmentValid = CharacterSet("fragment","/?") + CharacterSet::PCHAR;

    /*
     * RFC 3986 section 4.2
     *
     *  relative-ref  = relative-part [ "?" query ] [ "#" fragment ]
     *
     *  relative-part = "//" authority path-abempty
     *                / path-absolute
     *                / path-noscheme
     *                / path-empty
     */
    static const auto relativeRef = CharacterSet("relative-ref","/?#") + pathValid + queryValid + fragmentValid;
    return relativeRef;
}

/**
 * Governed by RFC 3986 section 2.1
 */
SBuf
AnyP::Uri::Encode(const SBuf &buf, const CharacterSet &ignore)
{
    if (buf.isEmpty())
        return buf;

    Parser::Tokenizer tk(buf);
    SBuf goodSection;
    // optimization for the arguably common "no encoding necessary" case
    if (tk.prefix(goodSection, ignore) && tk.atEnd())
        return buf;

    SBuf output;
    output.reserveSpace(buf.length() * 3); // worst case: encode all chars
    output.append(goodSection); // may be empty

    while (!tk.atEnd()) {
        // TODO: Add Tokenizer::parseOne(void).
        const auto ch = tk.remaining()[0];
        output.appendf("%%%02X", static_cast<unsigned int>(static_cast<unsigned char>(ch))); // TODO: Optimize using a table
        (void)tk.skip(ch);

        if (tk.prefix(goodSection, ignore))
            output.append(goodSection);
    }

    return output;
}

SBuf
AnyP::Uri::Decode(const SBuf &buf, const CharacterSet &ignore)
{
    SBuf output;
    Parser::Tokenizer tok(buf);
    while (!tok.atEnd()) {
        SBuf token;
        static const auto unencodedChars = CharacterSet("percent", "%").complement("unencoded");
        if (tok.prefix(token, unencodedChars))
            output.append(token);

        // we are either at '%' or at end of input
        const auto savePoint = tok.remaining();
        if (tok.skip('%')) {
            int64_t hex1 = 0, hex2 = 0;
            if (tok.int64(hex1, 16, false, 1) && tok.int64(hex2, 16, false, 1)) {
                const char found = static_cast<char>((hex1 << 4) | hex2);
                if (ignore[found])
                    output.append(savePoint.substr(0,3));
                else
                    output.append(found);
            } else
                throw TextException("invalid pct-encoded triplet", Here());
        }
    }
    return output;
}

const SBuf &
AnyP::Uri::Asterisk()
{
    static SBuf star("*");
    return star;
}

const SBuf &
AnyP::Uri::SlashPath()
{
    static SBuf slash("/");
    return slash;
}

void
AnyP::Uri::host(const char *src)
{
    hostAddr_.fromHost(src);
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

SBuf
AnyP::Uri::hostOrIp() const
{
    if (hostIsNumeric()) {
        static char ip[MAX_IPSTRLEN];
        const auto hostStrLen = hostIP().toHostStr(ip, sizeof(ip));
        return SBuf(ip, hostStrLen);
    } else
        return SBuf(host());
}

const SBuf &
AnyP::Uri::path() const
{
    // RFC 3986 section 3.3 says path can be empty (path-abempty).
    // RFC 7230 sections 2.7.3, 5.3.1, 5.7.2 - says path cannot be empty, default to "/"
    // at least when sending and using. We must still accept path-abempty as input.
    if (path_.isEmpty() && (scheme_ == AnyP::PROTO_HTTP || scheme_ == AnyP::PROTO_HTTPS))
        return SlashPath();

    return path_;
}

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

    assert(0 != matchDomainName("foo.com", ""));
    assert(0 != matchDomainName("foo.com", "", mdnHonorWildcards));
    assert(0 != matchDomainName("foo.com", "", mdnRejectSubsubDomains));

    /* more cases? */
}

/**
 * Extract the URI scheme and ':' delimiter from the given input buffer.
 *
 * Schemes up to 16 characters are accepted.
 *
 * Governed by RFC 3986 section 3.1
 */
static AnyP::UriScheme
uriParseScheme(Parser::Tokenizer &tok)
{
    /*
     * RFC 3986 section 3.1 paragraph 2:
     *
     * Scheme names consist of a sequence of characters beginning with a
     * letter and followed by any combination of letters, digits, plus
     * ("+"), period ("."), or hyphen ("-").
     */
    static const auto schemeChars = CharacterSet("scheme", "+.-") + CharacterSet::ALPHA + CharacterSet::DIGIT;

    SBuf str;
    if (tok.prefix(str, schemeChars, 16) && tok.skip(':') && CharacterSet::ALPHA[str.at(0)]) {
        const auto protocol = AnyP::UriScheme::FindProtocolType(str);
        if (protocol == AnyP::PROTO_UNKNOWN)
            return AnyP::UriScheme(protocol, str.c_str());
        return AnyP::UriScheme(protocol, nullptr);
    }

    throw TextException("invalid URI scheme", Here());
}

static void
fixPathWhitespace(SBuf &foundPath)
{
    const auto firstWsp = foundPath.findFirstOf(CharacterSet::WSP);
    if (firstWsp != SBuf::npos) {
        debugs(23, 2, "URI-path has whitespace: {" << foundPath << "}");
        switch (Config.uri_whitespace) {

        case URI_WHITESPACE_DENY:
            throw TextException("whitespace in URL path", Here());

        case URI_WHITESPACE_ENCODE: // done later
            break;

        case URI_WHITESPACE_CHOP:
            foundPath.chop(0, firstWsp);
            return;

        case URI_WHITESPACE_STRIP:
        default: {
            size_t dst = 0;
            for (size_t src = 0; src < foundPath.length(); ++src) {
               if (CharacterSet::WSP[foundPath[src]])
                   continue;
               if (src != dst)
                   foundPath.setAt(dst, foundPath[src]);
               ++dst;
            }
            if (dst != foundPath.length())
                foundPath.chop(0, dst); // truncate
            return;
        }
        }
    }
}

/*
 * Parse a URI/URL.
 *
 * It is assumed that the URL is complete -
 * ie, the end of the string is the end of the URL. Don't pass a partial
 * URL here as this routine doesn't have any way of knowing whether
 * it is partial or not (ie, it handles the case of no trailing slash as
 * being "end of host with implied path of /".
 *
 * method is used to switch parsers. If method is Http::METHOD_CONNECT,
 * then rather than a URL a hostname:port is looked for.
 */
bool
AnyP::Uri::parse(const HttpRequestMethod& method, const SBuf &rawUrl)
{
    try {
        int len = rawUrl.length();
        if (len + Config.appendDomainLen > (MAX_URL - 1))
            throw TextException(ToSBuf("URL too large (", len, " bytes)"), Here());

        if ((method == Http::METHOD_OPTIONS || method == Http::METHOD_TRACE) &&
                Asterisk().cmp(rawUrl) == 0) {
            // XXX: these methods might also occur in HTTPS traffic. Handle this better.
            setScheme(AnyP::PROTO_HTTP, nullptr);
            port(getScheme().defaultPort());
            path(Asterisk());
            return true;
        }

        Parser::Tokenizer tok(rawUrl);
        AnyP::UriScheme scheme;
        SBuf foundUserInfo;
        SBuf foundHost;
        int foundPort = -1;
        SBuf foundPath;

        if (method == Http::METHOD_CONNECT) {
            // For CONNECTs, RFC 9110 Section 9.3.6 requires "only the host and
            // port number of the tunnel destination, separated by a colon".
            parseAuthority(tok, foundHost, foundPort);

            if (foundPort == -1)
                throw TextException("missing required :port in CONNECT target", Here());

            if (!tok.atEnd())
                throw TextException("garbage after host:port in CONNECT target", Here());

        } else {
            scheme = uriParseScheme(tok);

            if (scheme == AnyP::PROTO_NONE)
                return false; // invalid scheme

            if (scheme == AnyP::PROTO_URN) {
                parseUrn(tok); // throws on any error
                return true;
            }

            // If the parsed scheme has no (known) default port, and there is no
            // explicit port, then we will reject the zero port during foundPort
            // validation, often resulting in a misleading 400/ERR_INVALID_URL.
            // TODO: Remove this hack
            foundPort = scheme.defaultPort().value_or(0); // may be reset later

            // URLs then have "//"
            static const SBuf doubleSlash("//");
            tok.skipRequired("authority-prefix",doubleSlash);

            /*
             * RFC 9110 section 4.2.4
             *
             *  Before making use of an "http" or "https" URI reference received
             *  from an untrusted source, a recipient SHOULD parse for userinfo
             *  and treat its presence as an error; it is likely being used to
             *  obscure the authority for the sake of phishing attacks.
             */
            const auto savePoint = tok.buf();
            (void)tok.prefix(foundUserInfo, UserInfoChars());
            if (!tok.skip('@')) {
                // no userinfo present
                tok.reset(savePoint);
                foundUserInfo.clear();
            } else {
                if (scheme == AnyP::PROTO_HTTP || scheme == AnyP::PROTO_HTTPS)
                    throw TextException(ToSBuf("forbidden userinfo@ found in ", scheme.image(), " URL"), Here());
                // normalize: strip encoding except for reserved ':' delimiter
                static const auto colon = CharacterSet("colon",":");
                foundUserInfo = Decode(foundUserInfo, colon);
            }

            // RFC 3986 authority section is mandatory when scheme is present
            parseAuthority(tok, foundHost, foundPort);
            // RFC 9110 section 4.2.1 and 4.2.2 require hostname, not allowing port-only authority section
            if (scheme == AnyP::PROTO_HTTP || scheme == AnyP::PROTO_HTTPS)
                throw TextException(ToSBuf("missing required hostname in ", scheme.image(), " URL"), Here());
            else {
                // Bug 3183 paranoid sanity check: If scheme is present, host must be too.
                Assure(!foundHost.isEmpty());
            }

            // TODO: separate the path?query#fragment sections
            foundPath = tok.remaining();
            if (!tok.skip('/')) {
                foundPath = SlashPath();
                foundPath.append(tok.remaining());
            }

            // normalize: decode all improperly encoded characters in the path?query#fragment area
            static const auto pathEncode = CharacterSet("path-delim","?#") + PathChars().complement();
            foundPath = Decode(foundPath, pathEncode);
        }

        debugs(23, 3, "Split URL '" << rawUrl << "' into proto='" << scheme.image() << "', host='" << foundHost << "', port='" << foundPort << "', path='" << foundPath << "'");

        // TODO: remove more of the RFC violations when possible
        fixPathWhitespace(foundPath);

        // all good. store the results
        setScheme(scheme);
        userInfo(foundUserInfo);
        host(foundHost.c_str());
        port(foundPort);
        path(foundPath);
        return true;

    } catch (...) {
        debugs(23, 2, "error: " << CurrentException << " " << Raw("rawUrl", rawUrl.rawContent(), rawUrl.length()));
        return false;
    }
}

/**
 * Governed by RFC 8141 section 2:
 *
 *  assigned-name = "urn" ":" NID ":" NSS
 *  NID           = (alphanum) 0*30(ldh) (alphanum)
 *  ldh           = alphanum / "-"
 *  NSS           = pchar *(pchar / "/")
 *
 * RFC 3986 Appendix D.2 defines (as deprecated):
 *
 *   alphanum     = ALPHA / DIGIT
 *
 * Notice that NID is exactly 2-32 characters in length.
 */
void
AnyP::Uri::parseUrn(Parser::Tokenizer &tok)
{
    static const auto nidChars = CharacterSet("NID","-") + CharacterSet::ALPHA + CharacterSet::DIGIT;
    static const auto alphanum = (CharacterSet::ALPHA + CharacterSet::DIGIT).rename("alphanum");
    SBuf nid;
    if (!tok.prefix(nid, nidChars, 32))
        throw TextException("NID not found", Here());

    if (!tok.skip(':'))
        throw TextException("NID too long or missing ':' delimiter", Here());

    if (nid.length() < 2)
        throw TextException("NID too short", Here());

    if (!alphanum[*nid.begin()])
        throw TextException("NID prefix is not alphanumeric", Here());

    if (!alphanum[*nid.rbegin()])
        throw TextException("NID suffix is not alphanumeric", Here());

    setScheme(AnyP::PROTO_URN, nullptr);
    host(nid.c_str());
    // TODO validate path characters
    path(tok.remaining());
    debugs(23, 3, "Split URI into proto=urn, nid=" << nid << ", " << Raw("path",path().rawContent(),path().length()));
}

/// Extracts a suspected but only partially validated; hostname and optional port.
void
AnyP::Uri::parseAuthority(Parser::Tokenizer &tok, SBuf &foundHost, int &foundPort) const
{
    foundHost = parseHost(tok);
    if (tok.skip(':'))
        foundPort = parsePort(tok);
}

/// Extracts and returns a (suspected but only partially validated) uri-host
/// IPv6address, IPv4address, or reg-name component. This function uses (and
/// quotes) RFC 3986, Section 3.2.2 syntax rules.
SBuf
AnyP::Uri::parseHost(Parser::Tokenizer &tok) const
{
    // host = IP-literal / IPv4address / reg-name

    // IP-literal = "[" ( IPv6address / IPvFuture  ) "]"
    if (tok.skip('[')) {
        // Add "." because IPv6address in RFC 3986 includes ls32, which includes
        // IPv4address: ls32 = ( h16 ":" h16 ) / IPv4address
        // This set rejects IPvFuture that needs a "v" character.
        static const CharacterSet IPv6chars = (
                CharacterSet::HEXDIG + CharacterSet("colon", ":") + CharacterSet("period", ".")).rename("IPv6");
        SBuf ipv6ish;
        if (!tok.prefix(ipv6ish, IPv6chars))
            throw TextException("malformed or unsupported bracketed IP address in uri-host", Here());

        if (!tok.skip(']'))
            throw TextException("IPv6 address is missing a closing bracket in uri-host", Here());

        // This rejects bracketed IPv4address and domain names because they lack ":".
        if (ipv6ish.find(':') == SBuf::npos)
            throw TextException("bracketed IPv6 address is missing a colon in uri-host", Here());

        // This rejects bracketed non-IP addresses that our caller would have
        // otherwise mistaken for a domain name (e.g., '[127.0.0:1]').
        Ip::Address ipv6check;
        if (!ipv6check.fromHost(ipv6ish.c_str()))
            throw TextException("malformed bracketed IPv6 address in uri-host", Here());

        return ipv6ish;
    }

    // no brackets implies we are looking at IPv4address or reg-name

    // XXX: This code does not detect/reject some bad host values (e.g.
    // "1.2.3.4.5"). TODO: Add more checks here.

    SBuf otherHost; // IPv4address-ish or reg-name-ish;
    // ":" is not in DomainNameChars() so we will stop before any port specification
    if (tok.prefix(otherHost, DomainNameChars())) {
        static const SBuf dot(".");

        // TODO: implement /etc/resolv.conf 'ndots' setting
        if (Config.appendDomain) {
            // For FQDN check for a dot.
            // For IPv4 addresses check for a dot.
            if (otherHost.find(dot) == SBuf::npos)
                otherHost.append(Config.appendDomain);
        }

        // remove (all) trailing dots from hostnames
        otherHost.trim(dot, false, true);

        // reject leading dot(s)
        if (otherHost.startsWith(dot))
            throw TextException("illegal hostname", Here());

        // reject repeated dot(s)
        static const SBuf twoDot("..");
        if (otherHost.find(twoDot) != SBuf::npos)
            throw TextException("illegal hostname", Here());

        // TODO: check RFC 1035 section 2.3.1 'label' requirements

        // RFC 1035 - hostnames should be treated as lower case to avoid security issues
        otherHost.toLower();
        return otherHost;
    }

    throw TextException("malformed IPv4 address or host name in uri-host", Here());
}

/// Extracts and returns an RFC 3986 URI authority port value (with additional
/// restrictions). The RFC defines port as a possibly empty sequence of decimal
/// digits. We reject certain ports (that are syntactically valid from the RFC
/// point of view) because we are worried that Squid and other traffic handlers
/// may dangerously mishandle unusual (and virtually always bogus) port numbers.
/// Rejected ports cannot be successfully used by Squid itself.
int
AnyP::Uri::parsePort(Parser::Tokenizer &tok) const
{
    if (tok.skip('0'))
        throw TextException("zero or zero-prefixed port", Here());

    int64_t rawPort = 0;
    if (!tok.int64(rawPort, 10, false)) // port = *DIGIT
        throw TextException("malformed or missing port", Here());

    Assure(rawPort > 0);
    constexpr KnownPort portMax = 65535; // TODO: Make this a class-scope constant and REuse it.
    constexpr auto portStorageMax = std::numeric_limits<Port::value_type>::max();
    static_assert(!Less(portStorageMax, portMax), "Port type can represent the maximum valid port number");
    if (Less(portMax, rawPort))
        throw TextException("huge port", Here());

    // TODO: Return KnownPort after migrating the non-CONNECT uri-host parsing
    // code to use us (so that foundPort "int" disappears or starts using Port).
    return NaturalCast<int>(rawPort);
}

void
AnyP::Uri::touch()
{
    absolute_.clear();
    authorityHttp_.clear();
    authorityWithPort_.clear();
}

SBuf &
AnyP::Uri::authority(bool requirePort) const
{
    if (authorityHttp_.isEmpty()) {

        // both formats contain Host/IP
        authorityWithPort_.append(host());
        authorityHttp_ = authorityWithPort_;

        if (port().has_value()) {
            authorityWithPort_.appendf(":%hu", *port());
            // authorityHttp_ only has :port for known non-default ports
            if (port() != getScheme().defaultPort())
                authorityHttp_ = authorityWithPort_;
        }
        // else XXX: We made authorityWithPort_ that does not have a port.
        // TODO: Audit callers and refuse to give out broken authorityWithPort_.
    }

    return requirePort ? authorityWithPort_ : authorityHttp_;
}

SBuf &
AnyP::Uri::absolute() const
{
    if (absolute_.isEmpty()) {
        // TODO: most URL will be much shorter, avoid allocating this much
        absolute_.reserveCapacity(MAX_URL);

        absolute_.append(getScheme().image());
        absolute_.append(":",1);
        if (getScheme() != AnyP::PROTO_URN) {
            absolute_.append("//", 2);
            const bool allowUserInfo = getScheme() == AnyP::PROTO_FTP ||
                                       getScheme() == AnyP::PROTO_UNKNOWN;

            if (allowUserInfo && !userInfo().isEmpty()) {
                static const CharacterSet uiChars = CharacterSet(UserInfoChars())
                                                    .remove('%')
                                                    .rename("userinfo-reserved");
                absolute_.append(Encode(userInfo(), uiChars));
                absolute_.append("@", 1);
            }
            absolute_.append(authority());
        } else {
            absolute_.append(host());
            absolute_.append(":", 1);
        }
        absolute_.append(Encode(path(), PathChars()));
    }

    return absolute_;
}

/* XXX: Performance: This is an *almost* duplicate of HttpRequest::effectiveRequestUri(). But elides the query-string.
 *        After copying it on in the first place! Would be less code to merge the two with a flag parameter.
 *        and never copy the query-string part in the first place
 */
char *
urlCanonicalCleanWithoutRequest(const SBuf &url, const HttpRequestMethod &method, const AnyP::UriScheme &scheme)
{
    LOCAL_ARRAY(char, buf, MAX_URL);

    snprintf(buf, sizeof(buf), SQUIDSBUFPH, SQUIDSBUFPRINT(url));
    buf[sizeof(buf)-1] = '\0';

    // URN, CONNECT method, and non-stripped URIs can go straight out
    if (Config.onoff.strip_query_terms && !(method == Http::METHOD_CONNECT || scheme == AnyP::PROTO_URN)) {
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
    return request->canonicalCleanUrl();
}

/**
 * Test if a URL is a relative reference.
 *
 * Governed by RFC 3986 section 4.2
 *
 *  relative-ref  = relative-part [ "?" query ] [ "#" fragment ]
 *
 *  relative-part = "//" authority path-abempty
 *                / path-absolute
 *                / path-noscheme
 *                / path-empty
 */
bool
urlIsRelative(const char *url)
{
    if (!url)
        return false; // no URL

    /*
     * RFC 3986 section 5.2.3
     *
     * path          = path-abempty    ; begins with "/" or is empty
     *               / path-absolute   ; begins with "/" but not "//"
     *               / path-noscheme   ; begins with a non-colon segment
     *               / path-rootless   ; begins with a segment
     *               / path-empty      ; zero characters
     */

    if (*url == '\0')
        return true; // path-empty

    if (*url == '/') {
        // network-path reference (a.k.a. 'scheme-relative URI') or
        // path-absolute (a.k.a. 'absolute-path reference')
        return true;
    }

    for (const auto *p = url; *p != '\0' && *p != '/' && *p != '?' && *p != '#'; ++p) {
        if (*p == ':')
            return false; // colon is forbidden in first segment
    }

    return true; // path-noscheme, path-abempty, path-rootless
}

void
AnyP::Uri::addRelativePath(const char *relUrl)
{
    // URN cannot be merged
    if (getScheme() == AnyP::PROTO_URN)
        return;

    // TODO: Handle . and .. segment normalization

    const auto lastSlashPos = path_.rfind('/');
    // TODO: To optimize and simplify, add and use SBuf::replace().
    const auto relUrlLength = strlen(relUrl);
    if (lastSlashPos == SBuf::npos) {
        // start replacing the whole path
        path_.reserveCapacity(1 + relUrlLength);
        path_.assign("/", 1);
    } else {
        // start replacing just the last segment
        path_.reserveCapacity(lastSlashPos + 1 + relUrlLength);
        path_.chop(0, lastSlashPos+1);
    }
    path_.append(relUrl, relUrlLength);
}

int
matchDomainName(const char *h, const char *d, MatchDomainNameFlags flags)
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
    if (dl == 0)
        return 1;

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
bool
urlCheckRequest(const HttpRequest * r)
{
    /* protocol "independent" methods
     *
     * actually these methods are specific to HTTP:
     * they are methods we receive on our HTTP port,
     * and if we had a FTP listener would not be relevant
     * there.
     *
     * So, we should delegate them to HTTP. The problem is that we
     * do not have a default protocol from the client side of HTTP.
     */

    if (r->method == Http::METHOD_CONNECT)
        return true;

    // we support OPTIONS and TRACE directed at us (with a 501 reply, for now)
    // we also support forwarding OPTIONS and TRACE, except for the *-URI ones
    if (r->method == Http::METHOD_OPTIONS || r->method == Http::METHOD_TRACE)
        return (r->header.getInt64(Http::HdrType::MAX_FORWARDS) == 0 || r->url.path() != AnyP::Uri::Asterisk());

    if (r->method == Http::METHOD_PURGE)
        return true;

    /* does method match the protocol? */
    switch (r->url.getScheme()) {

    case AnyP::PROTO_URN:
    case AnyP::PROTO_HTTP:
        return true;

    case AnyP::PROTO_FTP:
        if (r->method == Http::METHOD_PUT ||
                r->method == Http::METHOD_GET ||
                r->method == Http::METHOD_HEAD )
            return true;
        return false;

    case AnyP::PROTO_WAIS:
    case AnyP::PROTO_WHOIS:
        if (r->method == Http::METHOD_GET ||
                r->method == Http::METHOD_HEAD)
            return true;
        return false;

    case AnyP::PROTO_HTTPS:
#if USE_OPENSSL || USE_GNUTLS
        return true;
#else
        /*
         * Squid can't originate an SSL connection, so it should
         * never receive an "https:" URL.  It should always be
         * CONNECT instead.
         */
        return false;
#endif

    default:
        return false;
    }

    /* notreached */
    return false;
}

AnyP::Uri::Uri(AnyP::UriScheme const &aScheme) :
    scheme_(aScheme),
    hostIsNumeric_(false)
{
    *host_=0;
}

// TODO: fix code duplication with AnyP::Uri::parse()
char *
AnyP::Uri::cleanup(const char *uri)
{
    char *cleanedUri = nullptr;
    switch (Config.uri_whitespace) {
    case URI_WHITESPACE_ENCODE:
        cleanedUri = xstrndup(rfc1738_do_escape(uri, RFC1738_ESCAPE_UNESCAPED), MAX_URL);
        break;

    case URI_WHITESPACE_CHOP: {
        const auto pos = strcspn(uri, w_space);
        char *choppedUri = nullptr;
        if (pos < strlen(uri))
            choppedUri = xstrndup(uri, pos + 1);
        cleanedUri = xstrndup(rfc1738_do_escape(choppedUri ? choppedUri : uri,
                                                RFC1738_ESCAPE_UNESCAPED), MAX_URL);
        cleanedUri[pos] = '\0';
        xfree(choppedUri);
        break;
    }

    case URI_WHITESPACE_DENY:
    case URI_WHITESPACE_STRIP:
    default: {
        // TODO: avoid duplication with urlParse()
        const char *t;
        char *tmp_uri = static_cast<char*>(xmalloc(strlen(uri) + 1));
        char *q = tmp_uri;
        t = uri;
        while (*t) {
            if (!xisspace(*t)) {
                *q = *t;
                ++q;
            }
            ++t;
        }
        *q = '\0';
        cleanedUri = xstrndup(rfc1738_escape_unescaped(tmp_uri), MAX_URL);
        xfree(tmp_uri);
        break;
    }
    }

    assert(cleanedUri);
    return cleanedUri;
}

