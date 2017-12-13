/*
 * Copyright (C) 1996-2017 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID__URL_OLD_H
#define SQUID__URL_OLD_H

// TODO: refactor code using these url*() functions to use AnyP::Url instead

class HttpRequest;
class HttpRequestMethod;

void urlInitialize(void);
char *urlCanonicalClean(const HttpRequest *);
const char *urlCanonicalFakeHttps(const HttpRequest * request);
bool urlIsRelative(const char *);
char *urlMakeAbsolute(const HttpRequest *, const char *);
char *urlRInternal(const char *host, unsigned short port, const char *dir, const char *name);
char *urlInternal(const char *dir, const char *name);
int urlCheckRequest(const HttpRequest *);
char *urlHostname(const char *url);
void urlExtMethodConfigure(void);


// TODO: find a better home for matchDomainName()

enum MatchDomainNameFlags {
    mdnNone = 0,
    mdnHonorWildcards = 1 << 0,
    mdnRejectSubsubDomains = 1 << 1
};

/**
 * matchDomainName() matches a hostname (usually extracted from traffic)
 * with a domainname when mdnNone or mdnRejectSubsubDomains flags are used
 * according to the following rules:
 *
 *    HOST      |   DOMAIN    |   mdnNone | mdnRejectSubsubDomains
 * -------------|-------------|-----------|-----------------------
 *      foo.com |   foo.com   |     YES   |   YES
 *     .foo.com |   foo.com   |     YES   |   YES
 *    x.foo.com |   foo.com   |     NO    |   NO
 *      foo.com |  .foo.com   |     YES   |   YES
 *     .foo.com |  .foo.com   |     YES   |   YES
 *    x.foo.com |  .foo.com   |     YES   |   YES
 *   .x.foo.com |  .foo.com   |     YES   |   NO
 *  y.x.foo.com |  .foo.com   |     YES   |   NO
 *
 * if mdnHonorWildcards flag is set then the matchDomainName() also accepts
 * optional wildcards on hostname:
 *
 *    HOST      |    DOMAIN    |  MATCH?
 * -------------|--------------|-------
 *    *.foo.com |   x.foo.com  |   YES
 *    *.foo.com |  .x.foo.com  |   YES
 *    *.foo.com |    .foo.com  |   YES
 *    *.foo.com |     foo.com  |   NO
 *
 * The combination of mdnHonorWildcards and mdnRejectSubsubDomains flags is
 * supported.
 *
 * \retval 0 means the host matches the domain
 * \retval 1 means the host is greater than the domain
 * \retval -1 means the host is less than the domain
 */
int matchDomainName(const char *host, const char *domain, uint flags = mdnNone);

#endif /* SQUID__URL_OLD_H */

