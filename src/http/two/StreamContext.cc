/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "http/FrameType.h"
#include "http/two/FrameParser.h"
#include "http/two/StreamContext.h"

void
Http::Two::StreamContext::update(const Http2::FrameParserPointer &hp)
{
    if (!id)
        id = hp->frameStreamId();
    assert(id == hp->frameStreamId());

    switch (hp->frameType())
    {
    case Http::FRAME_HEADERS:
        if (state != Http2::CLOSED_REMOTE)
            state = Http2::OPEN;
        headers.append(hp->framePayload());
        break;

    case Http::FRAME_RST_STREAM:
        state = Http2::CLOSED;
        writeQueue.clear();
        break;

    default: // no state change
        break;
    }
}
