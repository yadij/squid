/*
 * Copyright (C) 1996-2017 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_SRC_HTTP_FORWARD_H
#define SQUID_SRC_HTTP_FORWARD_H

#include "http/one/forward.h"

#include <memory>
#include <utility>

namespace Http
{

class CommonStructure;
typedef std::shared_ptr<Http::CommonStructure> CommonStructurePointer;

/**
 * A delimiter pair used for common-structure based parsing.
 * First delimiter is the field-value separator.
 * Second delimiter is the key=value separator.
 * Either value being 0xFF indicates no delimiter.
 *
 * \note nil is not used as a delimiter because it can occur as a
 *     bare character within HTTP/1 headers whitespace or values.
 *     The same is not true for 0xFF.
 */
typedef std::pair<const char, const char> DelimiterPair;

class Message;
typedef RefCount<Http::Message> MessagePointer;

class Stream;
typedef RefCount<Http::Stream> StreamPointer;

} // namespace Http

// TODO move these classes into Http namespace
class HttpRequestMethod;
typedef RefCount<HttpRequestMethod> HttpRequestMethodPointer;

class HttpRequest;
typedef RefCount<HttpRequest> HttpRequestPointer;

class HttpReply;
typedef RefCount<HttpReply> HttpReplyPointer;

#endif /* SQUID_SRC_HTTP_FORWARD_H */

