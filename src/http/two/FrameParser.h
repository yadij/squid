/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_SRC_HTTP_TWO_FRAMEPARSER_H
#define SQUID_SRC_HTTP_TWO_FRAMEPARSER_H

#include "http/Parser.h"
#include "http/two/Frame.h"
#include "http/two/HpackTables.h"

namespace Http
{
namespace Two
{

/** HTTP/2 protocol frame parser
 *
 * Works on a raw character I/O buffer and tokenizes the content into
 * the frames of an HTTP/2 stream.
 *
 * \item magic-prefix (skip the connection setup prefix)
 * \item frame (a RFC7540 frame)
 */
class FrameParser : public Http::Parser
{
public:
    /* frame data accessors */
    Http::FrameType frameType() const {return fh_.type();}
    uint8_t frameFlags() const {return fh_.flags();}
    uint32_t frameStreamId() const {return fh_.streamId();}
    SBuf framePayload() const {return payload_;}
    SBuf framePriorities() const {return priorities_;}

    /* Http::Parser API */
    void clear() override;
    bool parse(const SBuf &) override;
    Http::Parser::size_type firstLineSize() const override {return sizeof(fh_.data);}

private:
    /// reset the parser state before next frame processing
    void resetFrame();
    bool decompressPayload();
    int32_t unpackInteger(::Parser::BinaryTokenizer &);
    SBuf unpackString(::Parser::BinaryTokenizer &);

    /// a copy of the last seen HTTP/2 frame header
    struct FrameHeader fh_;

    /// the last seen HTTP/2 frame payload
    SBuf payload_;

    /// the last seen HTTP/2 frame priority sequence (if any)
    SBuf priorities_;

    /// the HPACK decoder tables
    HpackTables hpackDecodeTables_;
};

} // namespace Two
} // namespace Http

#endif /* SQUID_SRC_HTTP_TWO_FRAMEPARSER_H */

