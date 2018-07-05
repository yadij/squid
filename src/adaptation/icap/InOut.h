/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_ICAPINOUT_H
#define SQUID_ICAPINOUT_H

#include "HttpReply.h"

// IcapInOut manages a pointer to the HTTP message being worked on.
// For HTTP responses, request header information is also available
// as the "cause". ICAP transactions use this class to store virgin
// and adapted HTTP messages.

namespace Adaptation
{
namespace Icap
{

class InOut
{

public:
    // TODO: s/Header/Message/i ?
    typedef Http::MessagePointer Header;

    void setCause(HttpRequest *r) {
        if (r) {
            cause = r;
        } else {
            assert(!cause);
        }
    }

    void setHeader(const Header &h) {
        header = h;
        body_pipe = header->body_pipe;
    }

    const HttpRequestPointer getRequest() const {
        if (cause)
            return cause;
        if (auto *h = dynamic_cast<HttpRequest*>(header.getRaw()))
            return HttpRequestPointer(h);
        return HttpRequestPointer();
    }

public:
    // virgin or adapted message being worked on
    Header header;   // parsed HTTP status/request line and headers

    // HTTP request header for HTTP responses (the cause of the response)
    HttpRequestPointer cause;

    // Copy of header->body_pipe, in case somebody moves the original.
    BodyPipe::Pointer body_pipe;
};

} // namespace Icap
} // namespace Adaptation

#endif /* SQUID_ICAPINOUT_H */

