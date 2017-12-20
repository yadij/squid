/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID__ANYP_URL_H
#define SQUID__ANYP_URL_H

#include "anyp/UriScheme.h"
#include "ip/Address.h"
#include "rfc2181.h"
#include "sbuf/SBuf.h"

#include <iosfwd>

namespace Parser {
class Tokenizer;
}

namespace AnyP {

/**
 * The URL class represents a Uniform Resource Location
 *
 * Governed by RFC 3986
 */
class Url
{
    MEMPROXY_CLASS(AnyP::Url);

public:
    Url() {*host_=0;}
    Url(AnyP::UriScheme const &aScheme) : scheme_(aScheme) { *host_=0; }
    Url(const AnyP::Url &other) {
        this->operator =(other);
    }
    AnyP::Url &operator =(const AnyP::Url &o) {
        scheme_ = o.scheme_;
        userInfo_ = o.userInfo_;
        memcpy(host_, o.host_, sizeof(host_));
        hostIsNumeric_ = o.hostIsNumeric_;
        hostAddr_ = o.hostAddr_;
        port_ = o.port_;
        path_ = o.path_;
        touch();
        return *this;
    }

    void clear() {
        scheme_=AnyP::PROTO_NONE;
        hostIsNumeric_ = false;
        *host_ = 0;
        hostAddr_.setEmpty();
        port_ = 0;
        touch();
    }
    void touch(); ///< clear the cached URI display forms

    bool parse(const HttpRequestMethod &, const char *url);

    AnyP::UriScheme const & getScheme() const {return scheme_;}

    /// convert the URL scheme to that given
    void setScheme(const AnyP::ProtocolType &p, const char *str) {
        scheme_ = AnyP::UriScheme(p, str);
        touch();
    }
    /// convert the URL scheme to that given
    void setScheme(const AnyP::UriScheme &s) {
        scheme_ = s;
        touch();
    }

    void userInfo(const SBuf &s) {userInfo_=s; touch();}
    const SBuf &userInfo() const {return userInfo_;}

    void host(const char *src);
    const char *host(void) const {return host_;}
    int hostIsNumeric(void) const {return hostIsNumeric_;}
    Ip::Address const & hostIP(void) const {return hostAddr_;}

    void port(unsigned short p) {port_=p; touch();}
    unsigned short port() const {return port_;}

    void path(const char *p) {path_=p; touch();}
    void path(const SBuf &p) {path_=p; touch();}
    const SBuf &path() const;

    /// the static '/' default URL-path
    static const SBuf &SlashPath();

    /// the static '*' pseudo-URL
    static const SBuf &Asterisk();

    /// the characters which truly are valid within URI
    static const CharacterSet &UriValidCharacters();

    /**
     * The authority-form URI for currently stored values.
     *
     * As defined by RFC 7230 section 5.3.3 this form omits the
     * userinfo@ field from RFC 3986 defined authority segment.
     *
     * \param requirePort when true the port will be included, otherwise
     *                    port will be elided when it is the default for
     *                    the current scheme.
     */
    SBuf &authority(bool requirePort = false) const;

    /**
     * The absolute-form URI for currently stored values.
     *
     * As defined by RFC 7230 section 5.3.3 this form omits the
     * userinfo@ field from RFC 3986 defined authority segments
     * when the protocol scheme is http: or https:.
     */
    SBuf &absolute() const;

private:
    bool parseScheme(Parser::Tokenizer &);
    void parseFinish(const char *const, const char *const, const SBuf &, const int);

    /**
     \par
     * The scheme of this URL. This has the 'type code' smell about it.
     * In future we may want to make the methods that dispatch based on
     * the scheme virtual and have a class per protocol.
     \par
     * On the other hand, having Protocol as an explicit concept is useful,
     * see for instance the ACLProtocol acl type. One way to represent this
     * is to have one prototype URL with no host etc for each scheme,
     * another is to have an explicit scheme class, and then each URL class
     * could be a subclass of the scheme. Another way is one instance of
     * a AnyP::UriScheme class instance for each URL scheme we support, and one URL
     * class for each manner of treating the scheme : a Hierarchical URL, a
     * non-hierarchical URL etc.
     \par
     * Deferring the decision, its a type code for now. RBC 20060507.
     \par
     * In order to make taking any of these routes easy, scheme is private
     * and immutable, only settable at construction time,
     */
    AnyP::UriScheme scheme_;

    SBuf userInfo_; // aka 'URL-login'

    // XXX: uses char[] instead of SBuf to reduce performance regressions
    //      from c_str() since most code using this is not yet using SBuf
    char host_[SQUIDHOSTNAMELEN];   ///< string representation of the URI authority name or IP
    bool hostIsNumeric_ = false;    ///< whether the authority 'host' is a raw-IP
    Ip::Address hostAddr_;          ///< binary representation of the URI authority if it is a raw-IP

    unsigned short port_ = 0;       ///< URL port

    // XXX: for now includes query-string.
    SBuf path_;     ///< URL path segment

    // pre-assembled URL forms
    mutable SBuf authorityHttp_;     ///< RFC 7230 section 5.3.3 authority, maybe without default-port
    mutable SBuf authorityWithPort_; ///< RFC 7230 section 5.3.3 authority with explicit port
    mutable SBuf absolute_;          ///< RFC 7230 section 5.3.2 absolute-URI
};

} // namespace AnyP

inline std::ostream &
operator <<(std::ostream &os, const AnyP::Url &url)
{
    // none means explicit empty string for scheme.
    if (url.getScheme() != AnyP::PROTO_NONE)
        os << url.getScheme().image();
    os << ":";

    // no authority section on URN
    if (url.getScheme() != AnyP::PROTO_URN)
        os << "//" << url.authority();

    // path is what it is - including absent
    os << url.path();
    return os;
}

#endif /* SQUID__ANYP_URL_H */

