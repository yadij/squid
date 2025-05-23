/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 55    HTTP Header */

#include "squid.h"
#include "base/Assure.h"
#include "base/CharacterSet.h"
#include "base/EnumIterator.h"
#include "base/Raw.h"
#include "base64.h"
#include "globals.h"
#include "http/ContentLengthInterpreter.h"
#include "HttpHdrCc.h"
#include "HttpHdrContRange.h"
#include "HttpHdrScTarget.h" // also includes HttpHdrSc.h
#include "HttpHeader.h"
#include "HttpHeaderFieldStat.h"
#include "HttpHeaderStat.h"
#include "HttpHeaderTools.h"
#include "MemBuf.h"
#include "mgr/Registration.h"
#include "mime_header.h"
#include "sbuf/StringConvert.h"
#include "SquidConfig.h"
#include "StatHist.h"
#include "Store.h"
#include "StrList.h"
#include "time/gadgets.h"
#include "TimeOrTag.h"
#include "util.h"

#include <algorithm>
#include <array>

/* XXX: the whole set of API managing the entries vector should be rethought
 *      after the parse4r-ng effort is complete.
 */

/*
 * On naming conventions:
 *
 * HTTP/1.1 defines message-header as
 *
 * message-header = field-name ":" [ field-value ] CRLF
 * field-name     = token
 * field-value    = *( field-content | LWS )
 *
 * HTTP/1.1 does not give a name name a group of all message-headers in a message.
 * Squid 1.1 seems to refer to that group _plus_ start-line as "headers".
 *
 * HttpHeader is an object that represents all message-headers in a message.
 * HttpHeader does not manage start-line.
 *
 * HttpHeader is implemented as a collection of header "entries".
 * An entry is a (field_id, field_name, field_value) triplet.
 */

/*
 * local constants and vars
 */

// statistics counters for headers. clients must not allow Http::HdrType::BAD_HDR to be counted
std::vector<HttpHeaderFieldStat> headerStatsTable(Http::HdrType::enumEnd_);

/* request-only headers. Used for cachemgr */
static HttpHeaderMask RequestHeadersMask;   /* set run-time using RequestHeaders */

/* reply-only headers. Used for cachemgr */
static HttpHeaderMask ReplyHeadersMask;     /* set run-time using ReplyHeaders */

/* header accounting */
// NP: keep in sync with enum http_hdr_owner_type
static std::array<HttpHeaderStat, hoEnd> HttpHeaderStats = {{
        HttpHeaderStat(/*hoNone*/ "all", nullptr),
#if USE_HTCP
        HttpHeaderStat(/*hoHtcpReply*/ "HTCP reply", &ReplyHeadersMask),
#endif
        HttpHeaderStat(/*hoRequest*/ "request", &RequestHeadersMask),
        HttpHeaderStat(/*hoReply*/ "reply", &ReplyHeadersMask)
#if USE_OPENSSL
        , HttpHeaderStat(/*hoErrorDetail*/ "error detail templates", nullptr)
#endif
        /* hoEnd */
    }
};

static int HeaderEntryParsedCount = 0;

/*
 * forward declarations and local routines
 */

class StoreEntry;

// update parse statistics for header id; if error is true also account
// for errors and write to debug log what happened
static void httpHeaderNoteParsedEntry(Http::HdrType id, String const &value, bool error);
static void httpHeaderStatDump(const HttpHeaderStat * hs, StoreEntry * e);
/** store report about current header usage and other stats */
static void httpHeaderStoreReport(StoreEntry * e);

/*
 * Module initialization routines
 */

static void
httpHeaderRegisterWithCacheManager(void)
{
    Mgr::RegisterAction("http_headers",
                        "HTTP Header Statistics",
                        httpHeaderStoreReport, 0, 1);
}

void
httpHeaderInitModule(void)
{
    /* check that we have enough space for masks */
    assert(8 * sizeof(HttpHeaderMask) >= Http::HdrType::enumEnd_);

    // masks are needed for stats page still
    for (auto h : WholeEnum<Http::HdrType>()) {
        if (Http::HeaderLookupTable.lookup(h).request)
            CBIT_SET(RequestHeadersMask,h);
        if (Http::HeaderLookupTable.lookup(h).reply)
            CBIT_SET(ReplyHeadersMask,h);
    }

    assert(HttpHeaderStats[0].label && "httpHeaderInitModule() called via main()");
    assert(HttpHeaderStats[hoEnd-1].label && "HttpHeaderStats created with all elements");

    /* init dependent modules */
    httpHdrScInitModule();

    httpHeaderRegisterWithCacheManager();
}

/*
 * HttpHeader Implementation
 */

HttpHeader::HttpHeader(const http_hdr_owner_type anOwner): owner(anOwner), len(0), conflictingContentLength_(false)
{
    assert(anOwner > hoNone && anOwner < hoEnd);
    debugs(55, 7, "init-ing hdr: " << this << " owner: " << owner);
    entries.reserve(32);
    httpHeaderMaskInit(&mask, 0);
}

// XXX: Delete as unused, expensive, and violating copy semantics by skipping Warnings
HttpHeader::HttpHeader(const HttpHeader &other): owner(other.owner), len(other.len), conflictingContentLength_(false)
{
    entries.reserve(other.entries.capacity());
    httpHeaderMaskInit(&mask, 0);
    update(&other); // will update the mask as well
}

HttpHeader::~HttpHeader()
{
    clean();
}

// XXX: Delete as unused, expensive, and violating assignment semantics by skipping Warnings
HttpHeader &
HttpHeader::operator =(const HttpHeader &other)
{
    if (this != &other) {
        // we do not really care, but the caller probably does
        assert(owner == other.owner);
        clean();
        update(&other); // will update the mask as well
        len = other.len;
        conflictingContentLength_ = other.conflictingContentLength_;
        teUnsupported_ = other.teUnsupported_;
    }
    return *this;
}

void
HttpHeader::clean()
{

    assert(owner > hoNone && owner < hoEnd);
    debugs(55, 7, "cleaning hdr: " << this << " owner: " << owner);

    if (owner <= hoReply) {
        /*
         * An unfortunate bug.  The entries array is initialized
         * such that count is set to zero.  httpHeaderClean() seems to
         * be called both when 'hdr' is created, and destroyed.  Thus,
         * we accumulate a large number of zero counts for 'hdr' before
         * it is ever used.  Can't think of a good way to fix it, except
         * adding a state variable that indicates whether or not 'hdr'
         * has been used.  As a hack, just never count zero-sized header
         * arrays.
         */
        if (!entries.empty())
            HttpHeaderStats[owner].hdrUCountDistr.count(entries.size());

        ++ HttpHeaderStats[owner].destroyedCount;

        HttpHeaderStats[owner].busyDestroyedCount += entries.size() > 0;
    } // if (owner <= hoReply)

    for (HttpHeaderEntry *e : entries) {
        if (e == nullptr)
            continue;
        if (!Http::any_valid_header(e->id)) {
            debugs(55, DBG_CRITICAL, "ERROR: Squid BUG: invalid entry (" << e->id << "). Ignored.");
        } else {
            if (owner <= hoReply)
                HttpHeaderStats[owner].fieldTypeDistr.count(e->id);
            delete e;
        }
    }

    entries.clear();
    httpHeaderMaskInit(&mask, 0);
    len = 0;
    conflictingContentLength_ = false;
    teUnsupported_ = false;
}

/* append entries (also see httpHeaderUpdate) */
void
HttpHeader::append(const HttpHeader * src)
{
    assert(src);
    assert(src != this);
    debugs(55, 7, "appending hdr: " << this << " += " << src);

    for (auto e : src->entries) {
        if (e)
            addEntry(e->clone());
    }
}

bool
HttpHeader::needUpdate(HttpHeader const *fresh) const
{
    for (const auto e: fresh->entries) {
        if (!e || skipUpdateHeader(e->id))
            continue;
        String value;
        if (!hasNamed(e->name, &value) ||
                (value != fresh->getByName(e->name)))
            return true;
    }
    return false;
}

bool
HttpHeader::skipUpdateHeader(const Http::HdrType id) const
{
    return
        // TODO: Consider updating Vary headers after comparing the magnitude of
        // the required changes (and/or cache losses) with compliance gains.
        (id == Http::HdrType::VARY);
}

void
HttpHeader::update(HttpHeader const *fresh)
{
    assert(fresh);
    assert(this != fresh);

    const HttpHeaderEntry *e;
    HttpHeaderPos pos = HttpHeaderInitPos;

    while ((e = fresh->getEntry(&pos))) {
        /* deny bad guys (ok to check for Http::HdrType::OTHER) here */

        if (skipUpdateHeader(e->id))
            continue;

        if (e->id != Http::HdrType::OTHER)
            delById(e->id);
        else
            delByName(e->name);
    }

    pos = HttpHeaderInitPos;
    while ((e = fresh->getEntry(&pos))) {
        /* deny bad guys (ok to check for Http::HdrType::OTHER) here */

        if (skipUpdateHeader(e->id))
            continue;

        debugs(55, 7, "Updating header '" << Http::HeaderLookupTable.lookup(e->id).name << "' in cached entry");

        addEntry(e->clone());
    }
}

bool
HttpHeader::Isolate(const char **parse_start, size_t l, const char **blk_start, const char **blk_end)
{
    /*
     * parse_start points to the first line of HTTP message *headers*,
     * not including the request or status lines
     */
    const size_t end = headersEnd(*parse_start, l);

    if (end) {
        *blk_start = *parse_start;
        *blk_end = *parse_start + end - 1;
        assert(**blk_end == '\n');
        // Point blk_end to the first character after the last header field.
        // In other words, blk_end should point to the CR?LF header terminator.
        if (end > 1 && *(*blk_end - 1) == '\r')
            --(*blk_end);
        *parse_start += end;
    }
    return end;
}

int
HttpHeader::parse(const char *buf, size_t buf_len, bool atEnd, size_t &hdr_sz, Http::ContentLengthInterpreter &clen)
{
    const char *parse_start = buf;
    const char *blk_start, *blk_end;
    hdr_sz = 0;

    if (!Isolate(&parse_start, buf_len, &blk_start, &blk_end)) {
        // XXX: do not parse non-isolated headers even if the connection is closed.
        // Treat unterminated headers as "partial headers" framing errors.
        if (!atEnd)
            return 0;
        blk_start = parse_start;
        blk_end = blk_start + strlen(blk_start);
    }

    if (parse(blk_start, blk_end - blk_start, clen)) {
        hdr_sz = parse_start - buf;
        return 1;
    }
    return -1;
}

// XXX: callers treat this return as boolean.
// XXX: A better mechanism is needed to signal different types of error.
//      lexicon, syntax, semantics, validation, access policy - are all (ab)using 'return 0'
int
HttpHeader::parse(const char *header_start, size_t hdrLen, Http::ContentLengthInterpreter &clen)
{
    const char *field_ptr = header_start;
    const char *header_end = header_start + hdrLen; // XXX: remove
    int warnOnError = (Config.onoff.relaxed_header_parser <= 0 ? DBG_IMPORTANT : 2);

    assert(header_start && header_end);
    debugs(55, 7, "parsing hdr: (" << this << ")" << std::endl << getStringPrefix(header_start, hdrLen));
    ++ HttpHeaderStats[owner].parsedCount;

    char *nulpos;
    if ((nulpos = (char*)memchr(header_start, '\0', hdrLen))) {
        debugs(55, DBG_IMPORTANT, "WARNING: HTTP header contains NULL characters {" <<
               getStringPrefix(header_start, nulpos-header_start) << "}\nNULL\n{" << getStringPrefix(nulpos+1, hdrLen-(nulpos-header_start)-1));
        clean();
        return 0;
    }

    /* common format headers are "<name>:[ws]<value>" lines delimited by <CRLF>.
     * continuation lines start with a (single) space or tab */
    while (field_ptr < header_end) {
        const char *field_start = field_ptr;
        const char *field_end;

        const char *hasBareCr = nullptr;
        size_t lines = 0;
        do {
            const char *this_line = field_ptr;
            field_ptr = (const char *)memchr(field_ptr, '\n', header_end - field_ptr);
            ++lines;

            if (!field_ptr) {
                // missing <LF>
                clean();
                return 0;
            }

            field_end = field_ptr;

            ++field_ptr;    /* Move to next line */

            if (field_end > this_line && field_end[-1] == '\r') {
                --field_end;    /* Ignore CR LF */

                if (owner == hoRequest && field_end > this_line) {
                    bool cr_only = true;
                    for (const char *p = this_line; p < field_end && cr_only; ++p) {
                        if (*p != '\r')
                            cr_only = false;
                    }
                    if (cr_only) {
                        debugs(55, DBG_IMPORTANT, "SECURITY WARNING: Rejecting HTTP request with a CR+ "
                               "header field to prevent request smuggling attacks: {" <<
                               getStringPrefix(header_start, hdrLen) << "}");
                        clean();
                        return 0;
                    }
                }
            }

            /* Barf on stray CR characters */
            if (memchr(this_line, '\r', field_end - this_line)) {
                hasBareCr = "bare CR";
                debugs(55, warnOnError, "WARNING: suspicious CR characters in HTTP header {" <<
                       getStringPrefix(field_start, field_end-field_start) << "}");

                if (Config.onoff.relaxed_header_parser) {
                    char *p = (char *) this_line;   /* XXX Warning! This destroys original header content and violates specifications somewhat */

                    while ((p = (char *)memchr(p, '\r', field_end - p)) != nullptr) {
                        *p = ' ';
                        ++p;
                    }
                } else {
                    clean();
                    return 0;
                }
            }

            if (this_line + 1 == field_end && this_line > field_start) {
                debugs(55, warnOnError, "WARNING: Blank continuation line in HTTP header {" <<
                       getStringPrefix(header_start, hdrLen) << "}");
                clean();
                return 0;
            }
        } while (field_ptr < header_end && (*field_ptr == ' ' || *field_ptr == '\t'));

        if (field_start == field_end) {
            if (field_ptr < header_end) {
                debugs(55, warnOnError, "WARNING: unparsable HTTP header field near {" <<
                       getStringPrefix(field_start, hdrLen-(field_start-header_start)) << "}");
                clean();
                return 0;
            }

            break;      /* terminating blank line */
        }

        const auto e = HttpHeaderEntry::parse(field_start, field_end, owner);
        if (!e) {
            debugs(55, warnOnError, "WARNING: unparsable HTTP header field {" <<
                   getStringPrefix(field_start, field_end-field_start) << "}");
            debugs(55, warnOnError, " in {" << getStringPrefix(header_start, hdrLen) << "}");

            clean();
            return 0;
        }

        if (lines > 1 || hasBareCr) {
            const auto framingHeader = (e->id == Http::HdrType::CONTENT_LENGTH || e->id == Http::HdrType::TRANSFER_ENCODING);
            if (framingHeader) {
                if (!hasBareCr) // already warned about bare CRs
                    debugs(55, warnOnError, "WARNING: obs-fold in framing-sensitive " << e->name << ": " << e->value);
                delete e;
                clean();
                return 0;
            }
        }

        if (e->id == Http::HdrType::CONTENT_LENGTH && !clen.checkField(e->value)) {
            delete e;

            if (Config.onoff.relaxed_header_parser)
                continue; // clen has printed any necessary warnings

            clean();
            return 0;
        }

        addEntry(e);
    }

    if (clen.headerWideProblem) {
        debugs(55, warnOnError, "WARNING: " << clen.headerWideProblem <<
               " Content-Length field values in" <<
               Raw("header", header_start, hdrLen));
    }

    String rawTe;
    if (clen.prohibitedAndIgnored()) {
        // prohibitedAndIgnored() includes trailer header blocks
        // being parsed as a case to forbid/ignore these headers.

        // RFC 7230 section 3.3.2: A server MUST NOT send a Content-Length
        // header field in any response with a status code of 1xx (Informational)
        // or 204 (No Content). And RFC 7230 3.3.3#1 tells recipients to ignore
        // such Content-Lengths.
        if (delById(Http::HdrType::CONTENT_LENGTH))
            debugs(55, 3, "Content-Length is " << clen.prohibitedAndIgnored());

        // The same RFC 7230 3.3.3#1-based logic applies to Transfer-Encoding
        // banned by RFC 7230 section 3.3.1.
        if (delById(Http::HdrType::TRANSFER_ENCODING))
            debugs(55, 3, "Transfer-Encoding is " << clen.prohibitedAndIgnored());

    } else if (getByIdIfPresent(Http::HdrType::TRANSFER_ENCODING, &rawTe)) {
        // RFC 2616 section 4.4: ignore Content-Length with Transfer-Encoding
        // RFC 7230 section 3.3.3 #3: Transfer-Encoding overwrites Content-Length
        delById(Http::HdrType::CONTENT_LENGTH);
        // and clen state becomes irrelevant

        if (rawTe.caseCmp("chunked") == 0) {
            ; // leave header present for chunked() method
        } else if (rawTe.caseCmp("identity") == 0) { // deprecated. no coding
            delById(Http::HdrType::TRANSFER_ENCODING);
        } else {
            // This also rejects multiple encodings until we support them properly.
            debugs(55, warnOnError, "WARNING: unsupported Transfer-Encoding used by client: " << rawTe);
            teUnsupported_ = true;
        }

    } else if (clen.sawBad) {
        // ensure our callers do not accidentally see bad Content-Length values
        delById(Http::HdrType::CONTENT_LENGTH);
        conflictingContentLength_ = true; // TODO: Rename to badContentLength_.
    } else if (clen.needsSanitizing) {
        // RFC 7230 section 3.3.2: MUST either reject or ... [sanitize];
        // ensure our callers see a clean Content-Length value or none at all
        delById(Http::HdrType::CONTENT_LENGTH);
        if (clen.sawGood) {
            putInt64(Http::HdrType::CONTENT_LENGTH, clen.value);
            debugs(55, 5, "sanitized Content-Length to be " << clen.value);
        }
    }

    return 1;           /* even if no fields where found, it is a valid header */
}

/* packs all the entries using supplied packer */
void
HttpHeader::packInto(Packable * p, bool mask_sensitive_info) const
{
    HttpHeaderPos pos = HttpHeaderInitPos;
    const HttpHeaderEntry *e;
    assert(p);
    debugs(55, 7, this << " into " << p <<
           (mask_sensitive_info ? " while masking" : ""));
    /* pack all entries one by one */
    while ((e = getEntry(&pos))) {
        if (!mask_sensitive_info) {
            e->packInto(p);
            continue;
        }

        bool maskThisEntry = false;
        switch (e->id) {
        case Http::HdrType::AUTHORIZATION:
        case Http::HdrType::PROXY_AUTHORIZATION:
            maskThisEntry = true;
            break;

        case Http::HdrType::FTP_ARGUMENTS:
            if (const HttpHeaderEntry *cmd = findEntry(Http::HdrType::FTP_COMMAND))
                maskThisEntry = (cmd->value == "PASS");
            break;

        default:
            break;
        }
        if (maskThisEntry) {
            p->append(e->name.rawContent(), e->name.length());
            p->append(": ** NOT DISPLAYED **\r\n", 23);
        } else {
            e->packInto(p);
        }

    }
    /* Pack in the "special" entries */

    /* Cache-Control */
}

/* returns next valid entry */
HttpHeaderEntry *
HttpHeader::getEntry(HttpHeaderPos * pos) const
{
    assert(pos);
    assert(*pos >= HttpHeaderInitPos && *pos < static_cast<ssize_t>(entries.size()));

    for (++(*pos); *pos < static_cast<ssize_t>(entries.size()); ++(*pos)) {
        if (entries[*pos])
            return static_cast<HttpHeaderEntry*>(entries[*pos]);
    }

    return nullptr;
}

/*
 * returns a pointer to a specified entry if any
 * note that we return one entry so it does not make much sense to ask for
 * "list" headers
 */
HttpHeaderEntry *
HttpHeader::findEntry(Http::HdrType id) const
{
    assert(any_registered_header(id));
    assert(!Http::HeaderLookupTable.lookup(id).list);

    /* check mask first */

    if (!CBIT_TEST(mask, id))
        return nullptr;

    /* looks like we must have it, do linear search */
    for (auto e : entries) {
        if (e && e->id == id)
            return e;
    }

    /* hm.. we thought it was there, but it was not found */
    assert(false);
    return nullptr;        /* not reached */
}

/*
 * same as httpHeaderFindEntry
 */
HttpHeaderEntry *
HttpHeader::findLastEntry(Http::HdrType id) const
{
    assert(any_registered_header(id));
    assert(!Http::HeaderLookupTable.lookup(id).list);

    /* check mask first */
    if (!CBIT_TEST(mask, id))
        return nullptr;

    for (auto e = entries.rbegin(); e != entries.rend(); ++e) {
        if (*e && (*e)->id == id)
            return *e;
    }

    /* hm.. we thought it was there, but it was not found */
    assert(false);
    return nullptr; /* not reached */
}

int
HttpHeader::delByName(const SBuf &name)
{
    int count = 0;
    HttpHeaderPos pos = HttpHeaderInitPos;
    httpHeaderMaskInit(&mask, 0);   /* temporal inconsistency */
    debugs(55, 9, "deleting '" << name << "' fields in hdr " << this);

    while (const HttpHeaderEntry *e = getEntry(&pos)) {
        if (!e->name.caseCmp(name))
            delAt(pos, count);
        else
            CBIT_SET(mask, e->id);
    }

    return count;
}

/* deletes all entries with a given id, returns the #entries deleted */
int
HttpHeader::delById(Http::HdrType id)
{
    debugs(55, 8, this << " del-by-id " << id);
    assert(any_registered_header(id));

    if (!CBIT_TEST(mask, id))
        return 0;

    int count = 0;

    HttpHeaderPos pos = HttpHeaderInitPos;
    while (HttpHeaderEntry *e = getEntry(&pos)) {
        if (e->id == id)
            delAt(pos, count); // deletes e
    }

    CBIT_CLR(mask, id);
    assert(count);
    return count;
}

/*
 * deletes an entry at pos and leaves a gap; leaving a gap makes it
 * possible to iterate(search) and delete fields at the same time
 * NOTE: Does not update the header mask. Caller must follow up with
 * a call to refreshMask() if headers_deleted was incremented.
 */
void
HttpHeader::delAt(HttpHeaderPos pos, int &headers_deleted)
{
    HttpHeaderEntry *e;
    assert(pos >= HttpHeaderInitPos && pos < static_cast<ssize_t>(entries.size()));
    e = static_cast<HttpHeaderEntry*>(entries[pos]);
    entries[pos] = nullptr;
    /* decrement header length, allow for ": " and crlf */
    len -= e->name.length() + 2 + e->value.size() + 2;
    assert(len >= 0);
    delete e;
    ++headers_deleted;
}

/*
 * Compacts the header storage
 */
void
HttpHeader::compact()
{
    // TODO: optimize removal, or possibly make it so that's not needed.
    entries.erase( std::remove(entries.begin(), entries.end(), nullptr),
                   entries.end());
}

/*
 * Refreshes the header mask. Required after delAt() calls.
 */
void
HttpHeader::refreshMask()
{
    httpHeaderMaskInit(&mask, 0);
    debugs(55, 7, "refreshing the mask in hdr " << this);
    for (auto e : entries) {
        if (e)
            CBIT_SET(mask, e->id);
    }
}

/* appends an entry;
 * does not call e->clone() so one should not reuse "*e"
 */
void
HttpHeader::addEntry(HttpHeaderEntry * e)
{
    assert(e);
    assert(any_HdrType_enum_value(e->id));
    assert(e->name.length());

    debugs(55, 7, this << " adding entry: " << e->id << " at " << entries.size());

    if (e->id != Http::HdrType::BAD_HDR) {
        if (CBIT_TEST(mask, e->id)) {
            ++ headerStatsTable[e->id].repCount;
        } else {
            CBIT_SET(mask, e->id);
        }
    }

    entries.push_back(e);

    len += e->length();
}

bool
HttpHeader::getList(Http::HdrType id, String *s) const
{
    debugs(55, 9, this << " joining for id " << id);
    /* only fields from ListHeaders array can be "listed" */
    assert(Http::HeaderLookupTable.lookup(id).list);

    if (!CBIT_TEST(mask, id))
        return false;

    for (auto e: entries) {
        if (e && e->id == id)
            strListAdd(s, e->value.termedBuf(), ',');
    }

    /*
     * note: we might get an empty (size==0) string if there was an "empty"
     * header. This results in an empty length String, which may have a NULL
     * buffer.
     */
    /* temporary warning: remove it? (Is it useful for diagnostics ?) */
    if (!s->size())
        debugs(55, 3, "empty list header: " << Http::HeaderLookupTable.lookup(id).name << "(" << id << ")");
    else
        debugs(55, 6, this << ": joined for id " << id << ": " << s);

    return true;
}

/* return a list of entries with the same id separated by ',' and ws */
String
HttpHeader::getList(Http::HdrType id) const
{
    HttpHeaderEntry *e;
    HttpHeaderPos pos = HttpHeaderInitPos;
    debugs(55, 9, this << "joining for id " << id);
    /* only fields from ListHeaders array can be "listed" */
    assert(Http::HeaderLookupTable.lookup(id).list);

    if (!CBIT_TEST(mask, id))
        return String();

    String s;

    while ((e = getEntry(&pos))) {
        if (e->id == id)
            strListAdd(&s, e->value.termedBuf(), ',');
    }

    /*
     * note: we might get an empty (size==0) string if there was an "empty"
     * header. This results in an empty length String, which may have a NULL
     * buffer.
     */
    /* temporary warning: remove it? (Is it useful for diagnostics ?) */
    if (!s.size())
        debugs(55, 3, "empty list header: " << Http::HeaderLookupTable.lookup(id).name << "(" << id << ")");
    else
        debugs(55, 6, this << ": joined for id " << id << ": " << s);

    return s;
}

/* return a string or list of entries with the same id separated by ',' and ws */
String
HttpHeader::getStrOrList(Http::HdrType id) const
{
    HttpHeaderEntry *e;

    if (Http::HeaderLookupTable.lookup(id).list)
        return getList(id);

    if ((e = findEntry(id)))
        return e->value;

    return String();
}

/*
 * Returns the value of the specified header and/or an undefined String.
 */
String
HttpHeader::getByName(const char *name) const
{
    String result;
    // ignore presence: return undefined string if an empty header is present
    (void)hasNamed(name, strlen(name), &result);
    return result;
}

String
HttpHeader::getByName(const SBuf &name) const
{
    String result;
    // ignore presence: return undefined string if an empty header is present
    (void)hasNamed(name, &result);
    return result;
}

String
HttpHeader::getById(Http::HdrType id) const
{
    String result;
    (void)getByIdIfPresent(id, &result);
    return result;
}

bool
HttpHeader::hasNamed(const SBuf &s, String *result) const
{
    return hasNamed(s.rawContent(), s.length(), result);
}

bool
HttpHeader::getByIdIfPresent(Http::HdrType id, String *result) const
{
    if (id == Http::HdrType::BAD_HDR)
        return false;
    if (!has(id))
        return false;
    if (result)
        *result = getStrOrList(id);
    return true;
}

bool
HttpHeader::hasNamed(const char *name, unsigned int namelen, String *result) const
{
    Http::HdrType id;
    HttpHeaderPos pos = HttpHeaderInitPos;
    HttpHeaderEntry *e;

    assert(name);

    /* First try the quick path */
    id = Http::HeaderLookupTable.lookup(name,namelen).id;

    if (id != Http::HdrType::BAD_HDR) {
        if (getByIdIfPresent(id, result))
            return true;
    }

    /* Sorry, an unknown header name. Do linear search */
    bool found = false;
    while ((e = getEntry(&pos))) {
        if (e->id == Http::HdrType::OTHER && e->name.length() == namelen && e->name.caseCmp(name, namelen) == 0) {
            found = true;
            if (!result)
                break;
            strListAdd(result, e->value.termedBuf(), ',');
        }
    }

    return found;
}

/*
 * Returns a the value of the specified list member, if any.
 */
SBuf
HttpHeader::getByNameListMember(const char *name, const char *member, const char separator) const
{
    assert(name);
    const auto header = getByName(name);
    return ::getListMember(header, member, separator);
}

/*
 * returns a the value of the specified list member, if any.
 */
SBuf
HttpHeader::getListMember(Http::HdrType id, const char *member, const char separator) const
{
    assert(any_registered_header(id));
    const auto header = getStrOrList(id);
    return ::getListMember(header, member, separator);
}

/* test if a field is present */
int
HttpHeader::has(Http::HdrType id) const
{
    assert(any_registered_header(id));
    debugs(55, 9, this << " lookup for " << id);
    return CBIT_TEST(mask, id);
}

void
HttpHeader::addVia(const AnyP::ProtocolVersion &ver, const HttpHeader *from)
{
    // TODO: do not add Via header for messages where Squid itself
    // generated the message (i.e., Downloader) there should be no Via header added at all.

    if (Config.onoff.via) {
        SBuf buf;
        // RFC 7230 section 5.7.1.: protocol-name is omitted when
        // the received protocol is HTTP.
        if (ver.protocol > AnyP::PROTO_NONE && ver.protocol < AnyP::PROTO_UNKNOWN &&
                ver.protocol != AnyP::PROTO_HTTP && ver.protocol != AnyP::PROTO_HTTPS)
            buf.appendf("%s/", AnyP::ProtocolType_str[ver.protocol]);
        buf.appendf("%d.%d %s", ver.major, ver.minor, ThisCache);
        const HttpHeader *hdr = from ? from : this;
        SBuf strVia = StringToSBuf(hdr->getList(Http::HdrType::VIA));
        if (!strVia.isEmpty())
            strVia.append(", ", 2);
        strVia.append(buf);
        updateOrAddStr(Http::HdrType::VIA, strVia);
    }
}

void
HttpHeader::putInt(Http::HdrType id, int number)
{
    assert(any_registered_header(id));
    assert(Http::HeaderLookupTable.lookup(id).type == Http::HdrFieldType::ftInt);  /* must be of an appropriate type */
    assert(number >= 0);
    addEntry(new HttpHeaderEntry(id, SBuf(), xitoa(number)));
}

void
HttpHeader::putInt64(Http::HdrType id, int64_t number)
{
    assert(any_registered_header(id));
    assert(Http::HeaderLookupTable.lookup(id).type == Http::HdrFieldType::ftInt64);    /* must be of an appropriate type */
    assert(number >= 0);
    addEntry(new HttpHeaderEntry(id, SBuf(), xint64toa(number)));
}

void
HttpHeader::putTime(Http::HdrType id, time_t htime)
{
    assert(any_registered_header(id));
    assert(Http::HeaderLookupTable.lookup(id).type == Http::HdrFieldType::ftDate_1123);    /* must be of an appropriate type */
    assert(htime >= 0);
    addEntry(new HttpHeaderEntry(id, SBuf(), Time::FormatRfc1123(htime)));
}

void
HttpHeader::putStr(Http::HdrType id, const char *str)
{
    assert(any_registered_header(id));
    assert(Http::HeaderLookupTable.lookup(id).type == Http::HdrFieldType::ftStr);  /* must be of an appropriate type */
    assert(str);
    addEntry(new HttpHeaderEntry(id, SBuf(), str));
}

void
HttpHeader::putAuth(const char *auth_scheme, const char *realm)
{
    assert(auth_scheme && realm);
    httpHeaderPutStrf(this, Http::HdrType::WWW_AUTHENTICATE, "%s realm=\"%s\"", auth_scheme, realm);
}

void
HttpHeader::putCc(const HttpHdrCc &cc)
{
    /* remove old directives if any */
    delById(Http::HdrType::CACHE_CONTROL);
    /* pack into mb */
    MemBuf mb;
    mb.init();
    cc.packInto(&mb);
    /* put */
    addEntry(new HttpHeaderEntry(Http::HdrType::CACHE_CONTROL, SBuf(), mb.buf));
    /* cleanup */
    mb.clean();
}

void
HttpHeader::putContRange(const HttpHdrContRange * cr)
{
    assert(cr);
    /* remove old directives if any */
    delById(Http::HdrType::CONTENT_RANGE);
    /* pack into mb */
    MemBuf mb;
    mb.init();
    httpHdrContRangePackInto(cr, &mb);
    /* put */
    addEntry(new HttpHeaderEntry(Http::HdrType::CONTENT_RANGE, SBuf(), mb.buf));
    /* cleanup */
    mb.clean();
}

void
HttpHeader::putRange(const HttpHdrRange * range)
{
    assert(range);
    /* remove old directives if any */
    delById(Http::HdrType::RANGE);
    /* pack into mb */
    MemBuf mb;
    mb.init();
    range->packInto(&mb);
    /* put */
    addEntry(new HttpHeaderEntry(Http::HdrType::RANGE, SBuf(), mb.buf));
    /* cleanup */
    mb.clean();
}

void
HttpHeader::putSc(HttpHdrSc *sc)
{
    assert(sc);
    /* remove old directives if any */
    delById(Http::HdrType::SURROGATE_CONTROL);
    /* pack into mb */
    MemBuf mb;
    mb.init();
    sc->packInto(&mb);
    /* put */
    addEntry(new HttpHeaderEntry(Http::HdrType::SURROGATE_CONTROL, SBuf(), mb.buf));
    /* cleanup */
    mb.clean();
}

/* add extension header (these fields are not parsed/analyzed/joined, etc.) */
void
HttpHeader::putExt(const char *name, const char *value)
{
    assert(name && value);
    debugs(55, 8, this << " adds ext entry " << name << " : " << value);
    addEntry(new HttpHeaderEntry(Http::HdrType::OTHER, SBuf(name), value));
}

void
HttpHeader::updateOrAddStr(const Http::HdrType id, const SBuf &newValue)
{
    assert(any_registered_header(id));
    assert(Http::HeaderLookupTable.lookup(id).type == Http::HdrFieldType::ftStr);

    // XXX: HttpHeaderEntry::value suffers from String size limits
    Assure(newValue.length() < String::SizeMaxXXX());

    if (!CBIT_TEST(mask, id)) {
        auto newValueCopy = newValue; // until HttpHeaderEntry::value becomes SBuf
        addEntry(new HttpHeaderEntry(id, SBuf(), newValueCopy.c_str()));
        return;
    }

    auto foundSameName = false;
    for (auto &e: entries) {
        if (!e || e->id != id)
            continue;

        if (foundSameName) {
            // get rid of this repeated same-name entry
            delete e;
            e = nullptr;
            continue;
        }

        if (newValue.cmp(e->value.termedBuf()) != 0)
            e->value.assign(newValue.rawContent(), newValue.plength());

        foundSameName = true;
        // continue to delete any repeated same-name entries
    }
    assert(foundSameName);
    debugs(55, 5, "synced: " << Http::HeaderLookupTable.lookup(id).name << ": " << newValue);
}

int
HttpHeader::getInt(Http::HdrType id) const
{
    assert(any_registered_header(id));
    assert(Http::HeaderLookupTable.lookup(id).type == Http::HdrFieldType::ftInt);  /* must be of an appropriate type */
    HttpHeaderEntry *e;

    if ((e = findEntry(id)))
        return e->getInt();

    return -1;
}

int64_t
HttpHeader::getInt64(Http::HdrType id) const
{
    assert(any_registered_header(id));
    assert(Http::HeaderLookupTable.lookup(id).type == Http::HdrFieldType::ftInt64);    /* must be of an appropriate type */
    HttpHeaderEntry *e;

    if ((e = findEntry(id)))
        return e->getInt64();

    return -1;
}

time_t
HttpHeader::getTime(Http::HdrType id) const
{
    HttpHeaderEntry *e;
    time_t value = -1;
    assert(any_registered_header(id));
    assert(Http::HeaderLookupTable.lookup(id).type == Http::HdrFieldType::ftDate_1123);    /* must be of an appropriate type */

    if ((e = findEntry(id))) {
        value = Time::ParseRfc1123(e->value.termedBuf());
        httpHeaderNoteParsedEntry(e->id, e->value, value < 0);
    }

    return value;
}

/* sync with httpHeaderGetLastStr */
const char *
HttpHeader::getStr(Http::HdrType id) const
{
    HttpHeaderEntry *e;
    assert(any_registered_header(id));
    assert(Http::HeaderLookupTable.lookup(id).type == Http::HdrFieldType::ftStr);  /* must be of an appropriate type */

    if ((e = findEntry(id))) {
        httpHeaderNoteParsedEntry(e->id, e->value, false);  /* no errors are possible */
        return e->value.termedBuf();
    }

    return nullptr;
}

/* unusual */
const char *
HttpHeader::getLastStr(Http::HdrType id) const
{
    HttpHeaderEntry *e;
    assert(any_registered_header(id));
    assert(Http::HeaderLookupTable.lookup(id).type == Http::HdrFieldType::ftStr);  /* must be of an appropriate type */

    if ((e = findLastEntry(id))) {
        httpHeaderNoteParsedEntry(e->id, e->value, false);  /* no errors are possible */
        return e->value.termedBuf();
    }

    return nullptr;
}

HttpHdrCc *
HttpHeader::getCc() const
{
    if (!CBIT_TEST(mask, Http::HdrType::CACHE_CONTROL))
        return nullptr;

    String s;
    getList(Http::HdrType::CACHE_CONTROL, &s);

    HttpHdrCc *cc=new HttpHdrCc();

    if (!cc->parse(s)) {
        delete cc;
        cc = nullptr;
    }

    ++ HttpHeaderStats[owner].ccParsedCount;

    if (cc)
        httpHdrCcUpdateStats(cc, &HttpHeaderStats[owner].ccTypeDistr);

    httpHeaderNoteParsedEntry(Http::HdrType::CACHE_CONTROL, s, !cc);

    return cc;
}

HttpHdrRange *
HttpHeader::getRange() const
{
    HttpHdrRange *r = nullptr;
    HttpHeaderEntry *e;
    /* some clients will send "Request-Range" _and_ *matching* "Range"
     * who knows, some clients might send Request-Range only;
     * this "if" should work correctly in both cases;
     * hopefully no clients send mismatched headers! */

    if ((e = findEntry(Http::HdrType::RANGE)) ||
            (e = findEntry(Http::HdrType::REQUEST_RANGE))) {
        r = HttpHdrRange::ParseCreate(&e->value);
        httpHeaderNoteParsedEntry(e->id, e->value, !r);
    }

    return r;
}

HttpHdrSc *
HttpHeader::getSc() const
{
    if (!CBIT_TEST(mask, Http::HdrType::SURROGATE_CONTROL))
        return nullptr;

    String s;

    (void) getList(Http::HdrType::SURROGATE_CONTROL, &s);

    HttpHdrSc *sc = httpHdrScParseCreate(s);

    ++ HttpHeaderStats[owner].ccParsedCount;

    if (sc)
        sc->updateStats(&HttpHeaderStats[owner].scTypeDistr);

    httpHeaderNoteParsedEntry(Http::HdrType::SURROGATE_CONTROL, s, !sc);

    return sc;
}

HttpHdrContRange *
HttpHeader::getContRange() const
{
    HttpHdrContRange *cr = nullptr;
    HttpHeaderEntry *e;

    if ((e = findEntry(Http::HdrType::CONTENT_RANGE))) {
        cr = httpHdrContRangeParseCreate(e->value.termedBuf());
        httpHeaderNoteParsedEntry(e->id, e->value, !cr);
    }

    return cr;
}

SBuf
HttpHeader::getAuthToken(Http::HdrType id, const char *auth_scheme) const
{
    const char *field;
    int l;
    assert(auth_scheme);
    field = getStr(id);

    static const SBuf nil;
    if (!field)         /* no authorization field */
        return nil;

    l = strlen(auth_scheme);

    if (!l || strncasecmp(field, auth_scheme, l))   /* wrong scheme */
        return nil;

    field += l;

    if (!xisspace(*field))  /* wrong scheme */
        return nil;

    /* skip white space */
    for (; field && xisspace(*field); ++field);

    if (!*field)        /* no authorization cookie */
        return nil;

    const auto fieldLen = strlen(field);
    SBuf result;
    char *decodedAuthToken = result.rawAppendStart(BASE64_DECODE_LENGTH(fieldLen));
    struct base64_decode_ctx ctx;
    base64_decode_init(&ctx);
    size_t decodedLen = 0;
    if (!base64_decode_update(&ctx, &decodedLen, reinterpret_cast<uint8_t*>(decodedAuthToken), fieldLen, field) ||
            !base64_decode_final(&ctx)) {
        return nil;
    }
    result.rawAppendFinish(decodedAuthToken, decodedLen);
    return result;
}

ETag
HttpHeader::getETag(Http::HdrType id) const
{
    ETag etag = {nullptr, -1};
    HttpHeaderEntry *e;
    assert(Http::HeaderLookupTable.lookup(id).type == Http::HdrFieldType::ftETag);     /* must be of an appropriate type */

    if ((e = findEntry(id)))
        etagParseInit(&etag, e->value.termedBuf());

    return etag;
}

TimeOrTag
HttpHeader::getTimeOrTag(Http::HdrType id) const
{
    TimeOrTag tot;
    HttpHeaderEntry *e;
    assert(Http::HeaderLookupTable.lookup(id).type == Http::HdrFieldType::ftDate_1123_or_ETag);    /* must be of an appropriate type */
    memset(&tot, 0, sizeof(tot));

    if ((e = findEntry(id))) {
        const char *str = e->value.termedBuf();
        /* try as an ETag */

        if (etagParseInit(&tot.tag, str)) {
            tot.valid = tot.tag.str != nullptr;
            tot.time = -1;
        } else {
            /* or maybe it is time? */
            tot.time = Time::ParseRfc1123(str);
            tot.valid = tot.time >= 0;
            tot.tag.str = nullptr;
        }
    }

    assert(tot.time < 0 || !tot.tag.str);   /* paranoid */
    return tot;
}

/*
 * HttpHeaderEntry
 */

HttpHeaderEntry::HttpHeaderEntry(Http::HdrType anId, const SBuf &aName, const char *aValue)
{
    assert(any_HdrType_enum_value(anId));
    id = anId;

    if (id != Http::HdrType::OTHER)
        name = Http::HeaderLookupTable.lookup(id).name;
    else
        name = aName;

    value = aValue;

    if (id != Http::HdrType::BAD_HDR)
        ++ headerStatsTable[id].aliveCount;

    debugs(55, 9, "created HttpHeaderEntry " << this << ": '" << name << " : " << value );
}

HttpHeaderEntry::~HttpHeaderEntry()
{
    debugs(55, 9, "destroying entry " << this << ": '" << name << ": " << value << "'");

    if (id != Http::HdrType::BAD_HDR) {
        assert(headerStatsTable[id].aliveCount);
        -- headerStatsTable[id].aliveCount;
        id = Http::HdrType::BAD_HDR; // it already is BAD_HDR, no sense in resetting it
    }

}

/* parses and inits header entry, returns true/false */
HttpHeaderEntry *
HttpHeaderEntry::parse(const char *field_start, const char *field_end, const http_hdr_owner_type msgType)
{
    /* note: name_start == field_start */
    const char *name_end = (const char *)memchr(field_start, ':', field_end - field_start);
    int name_len = name_end ? name_end - field_start :0;
    const char *value_start = field_start + name_len + 1;   /* skip ':' */
    /* note: value_end == field_end */

    ++ HeaderEntryParsedCount;

    /* do we have a valid field name within this field? */

    if (!name_len || name_end > field_end)
        return nullptr;

    if (name_len > 65534) {
        /* String must be LESS THAN 64K and it adds a terminating NULL */
        // TODO: update this to show proper name_len in Raw markup, but not print all that
        debugs(55, 2, "ignoring huge header field (" << Raw("field_start", field_start, 100) << "...)");
        return nullptr;
    }

    /*
     * RFC 7230 section 3.2.4:
     * "No whitespace is allowed between the header field-name and colon.
     * ...
     *  A server MUST reject any received request message that contains
     *  whitespace between a header field-name and colon with a response code
     *  of 400 (Bad Request).  A proxy MUST remove any such whitespace from a
     *  response message before forwarding the message downstream."
     */
    if (xisspace(field_start[name_len - 1])) {

        if (msgType == hoRequest)
            return nullptr;

        // for now, also let relaxed parser remove this BWS from any non-HTTP messages
        const bool stripWhitespace = (msgType == hoReply) ||
                                     Config.onoff.relaxed_header_parser;
        if (!stripWhitespace)
            return nullptr; // reject if we cannot strip

        debugs(55, Config.onoff.relaxed_header_parser <= 0 ? 1 : 2,
               "WARNING: Whitespace after header name in '" << getStringPrefix(field_start, field_end-field_start) << "'");

        while (name_len > 0 && xisspace(field_start[name_len - 1]))
            --name_len;

        if (!name_len) {
            debugs(55, 2, "found header with only whitespace for name");
            return nullptr;
        }
    }

    /* RFC 7230 section 3.2:
     *
     *  header-field   = field-name ":" OWS field-value OWS
     *  field-name     = token
     *  token          = 1*TCHAR
     */
    for (const char *pos = field_start; pos < (field_start+name_len); ++pos) {
        if (!CharacterSet::TCHAR[*pos]) {
            debugs(55, 2, "found header with invalid characters in " <<
                   Raw("field-name", field_start, min(name_len,100)) << "...");
            return nullptr;
        }
    }

    /* now we know we can parse it */

    debugs(55, 9, "parsing HttpHeaderEntry: near '" <<  getStringPrefix(field_start, field_end-field_start) << "'");

    /* is it a "known" field? */
    Http::HdrType id = Http::HeaderLookupTable.lookup(field_start,name_len).id;
    debugs(55, 9, "got hdr-id=" << id);

    SBuf theName;

    String value;

    if (id == Http::HdrType::BAD_HDR)
        id = Http::HdrType::OTHER;

    /* set field name */
    if (id == Http::HdrType::OTHER)
        theName.append(field_start, name_len);
    else
        theName = Http::HeaderLookupTable.lookup(id).name;

    /* trim field value */
    while (value_start < field_end && xisspace(*value_start))
        ++value_start;

    while (value_start < field_end && xisspace(field_end[-1]))
        --field_end;

    if (field_end - value_start > 65534) {
        /* String must be LESS THAN 64K and it adds a terminating NULL */
        debugs(55, 2, "WARNING: found '" << theName << "' header of " << (field_end - value_start) << " bytes");
        return nullptr;
    }

    /* set field value */
    value.assign(value_start, field_end - value_start);

    if (id != Http::HdrType::BAD_HDR)
        ++ headerStatsTable[id].seenCount;

    debugs(55, 9, "parsed HttpHeaderEntry: '" << theName << ": " << value << "'");

    return new HttpHeaderEntry(id, theName, value.termedBuf());
}

HttpHeaderEntry *
HttpHeaderEntry::clone() const
{
    return new HttpHeaderEntry(id, name, value.termedBuf());
}

void
HttpHeaderEntry::packInto(Packable * p) const
{
    assert(p);
    p->append(name.rawContent(), name.length());
    p->append(": ", 2);
    p->append(value.rawBuf(), value.size());
    p->append("\r\n", 2);
}

int
HttpHeaderEntry::getInt() const
{
    int val = -1;
    int ok = httpHeaderParseInt(value.termedBuf(), &val);
    httpHeaderNoteParsedEntry(id, value, ok == 0);
    /* XXX: Should we check ok - ie
     * return ok ? -1 : value;
     */
    return val;
}

int64_t
HttpHeaderEntry::getInt64() const
{
    int64_t val = -1;
    const bool ok = httpHeaderParseOffset(value.termedBuf(), &val);
    httpHeaderNoteParsedEntry(id, value, !ok);
    return val; // remains -1 if !ok (XXX: bad method API)
}

static void
httpHeaderNoteParsedEntry(Http::HdrType id, String const &context, bool error)
{
    if (id != Http::HdrType::BAD_HDR)
        ++ headerStatsTable[id].parsCount;

    if (error) {
        if (id != Http::HdrType::BAD_HDR)
            ++ headerStatsTable[id].errCount;
        debugs(55, 2, "cannot parse hdr field: '" << Http::HeaderLookupTable.lookup(id).name << ": " << context << "'");
    }
}

/*
 * Reports
 */

/* tmp variable used to pass stat info to dumpers */
extern const HttpHeaderStat *dump_stat;     /* argh! */
const HttpHeaderStat *dump_stat = nullptr;

static void
httpHeaderFieldStatDumper(StoreEntry * sentry, int, double val, double, int count)
{
    const int id = static_cast<int>(val);
    const bool valid_id = Http::any_valid_header(static_cast<Http::HdrType>(id));
    const char *name = valid_id ? Http::HeaderLookupTable.lookup(static_cast<Http::HdrType>(id)).name : "INVALID";
    int visible = count > 0;
    /* for entries with zero count, list only those that belong to current type of message */

    if (!visible && valid_id && dump_stat->owner_mask)
        visible = CBIT_TEST(*dump_stat->owner_mask, id);

    if (visible)
        storeAppendPrintf(sentry, "%2d\t %-20s\t %5d\t %6.2f\n",
                          id, name, count, xdiv(count, dump_stat->busyDestroyedCount));
}

static void
httpHeaderFldsPerHdrDumper(StoreEntry * sentry, int idx, double val, double, int count)
{
    if (count)
        storeAppendPrintf(sentry, "%2d\t %5d\t %5d\t %6.2f\n",
                          idx, (int) val, count,
                          xpercent(count, dump_stat->destroyedCount));
}

static void
httpHeaderStatDump(const HttpHeaderStat * hs, StoreEntry * e)
{
    assert(hs);
    assert(e);

    if (!hs->owner_mask)
        return; // these HttpHeaderStat objects were not meant to be dumped here

    dump_stat = hs;
    storeAppendPrintf(e, "\nHeader Stats: %s\n", hs->label);
    storeAppendPrintf(e, "\nField type distribution\n");
    storeAppendPrintf(e, "%2s\t %-20s\t %5s\t %6s\n",
                      "id", "name", "count", "#/header");
    hs->fieldTypeDistr.dump(e, httpHeaderFieldStatDumper);
    storeAppendPrintf(e, "\nCache-control directives distribution\n");
    storeAppendPrintf(e, "%2s\t %-20s\t %5s\t %6s\n",
                      "id", "name", "count", "#/cc_field");
    hs->ccTypeDistr.dump(e, httpHdrCcStatDumper);
    storeAppendPrintf(e, "\nSurrogate-control directives distribution\n");
    storeAppendPrintf(e, "%2s\t %-20s\t %5s\t %6s\n",
                      "id", "name", "count", "#/sc_field");
    hs->scTypeDistr.dump(e, httpHdrScStatDumper);
    storeAppendPrintf(e, "\nNumber of fields per header distribution\n");
    storeAppendPrintf(e, "%2s\t %-5s\t %5s\t %6s\n",
                      "id", "#flds", "count", "%total");
    hs->hdrUCountDistr.dump(e, httpHeaderFldsPerHdrDumper);
    storeAppendPrintf(e, "\n");
    dump_stat = nullptr;
}

void
httpHeaderStoreReport(StoreEntry * e)
{
    assert(e);

    HttpHeaderStats[0].parsedCount =
        HttpHeaderStats[hoRequest].parsedCount + HttpHeaderStats[hoReply].parsedCount;
    HttpHeaderStats[0].ccParsedCount =
        HttpHeaderStats[hoRequest].ccParsedCount + HttpHeaderStats[hoReply].ccParsedCount;
    HttpHeaderStats[0].destroyedCount =
        HttpHeaderStats[hoRequest].destroyedCount + HttpHeaderStats[hoReply].destroyedCount;
    HttpHeaderStats[0].busyDestroyedCount =
        HttpHeaderStats[hoRequest].busyDestroyedCount + HttpHeaderStats[hoReply].busyDestroyedCount;

    for (const auto &stats: HttpHeaderStats)
        httpHeaderStatDump(&stats, e);

    /* field stats for all messages */
    storeAppendPrintf(e, "\nHttp Fields Stats (replies and requests)\n");

    storeAppendPrintf(e, "%2s\t %-25s\t %5s\t %6s\t %6s\n",
                      "id", "name", "#alive", "%err", "%repeat");

    // scan heaaderTable and output
    for (auto h : WholeEnum<Http::HdrType>()) {
        auto stats = headerStatsTable[h];
        storeAppendPrintf(e, "%2d\t %-25s\t %5d\t %6.3f\t %6.3f\n",
                          Http::HeaderLookupTable.lookup(h).id,
                          Http::HeaderLookupTable.lookup(h).name,
                          stats.aliveCount,
                          xpercent(stats.errCount, stats.parsCount),
                          xpercent(stats.repCount, stats.seenCount));
    }

    storeAppendPrintf(e, "Headers Parsed: %d + %d = %d\n",
                      HttpHeaderStats[hoRequest].parsedCount,
                      HttpHeaderStats[hoReply].parsedCount,
                      HttpHeaderStats[0].parsedCount);
    storeAppendPrintf(e, "Hdr Fields Parsed: %d\n", HeaderEntryParsedCount);
}

int
HttpHeader::hasListMember(Http::HdrType id, const char *member, const char separator) const
{
    int result = 0;
    const char *pos = nullptr;
    const char *item;
    int ilen;
    int mlen = strlen(member);

    assert(any_registered_header(id));

    String header (getStrOrList(id));

    while (strListGetItem(&header, separator, &item, &ilen, &pos)) {
        if (strncasecmp(item, member, mlen) == 0
                && (item[mlen] == '=' || item[mlen] == separator || item[mlen] == ';' || item[mlen] == '\0')) {
            result = 1;
            break;
        }
    }

    return result;
}

int
HttpHeader::hasByNameListMember(const char *name, const char *member, const char separator) const
{
    int result = 0;
    const char *pos = nullptr;
    const char *item;
    int ilen;
    int mlen = strlen(member);

    assert(name);

    String header (getByName(name));

    while (strListGetItem(&header, separator, &item, &ilen, &pos)) {
        if (strncasecmp(item, member, mlen) == 0
                && (item[mlen] == '=' || item[mlen] == separator || item[mlen] == ';' || item[mlen] == '\0')) {
            result = 1;
            break;
        }
    }

    return result;
}

void
HttpHeader::removeHopByHopEntries()
{
    removeConnectionHeaderEntries();

    const HttpHeaderEntry *e;
    HttpHeaderPos pos = HttpHeaderInitPos;
    int headers_deleted = 0;
    while ((e = getEntry(&pos))) {
        Http::HdrType id = e->id;
        if (Http::HeaderLookupTable.lookup(id).hopbyhop) {
            delAt(pos, headers_deleted);
            CBIT_CLR(mask, id);
        }
    }
}

void
HttpHeader::removeConnectionHeaderEntries()
{
    if (has(Http::HdrType::CONNECTION)) {
        /* anything that matches Connection list member will be deleted */
        String strConnection;

        (void) getList(Http::HdrType::CONNECTION, &strConnection);
        const HttpHeaderEntry *e;
        HttpHeaderPos pos = HttpHeaderInitPos;
        /*
         * think: on-average-best nesting of the two loops (hdrEntry
         * and strListItem) @?@
         */
        /*
         * maybe we should delete standard stuff ("keep-alive","close")
         * from strConnection first?
         */

        int headers_deleted = 0;
        while ((e = getEntry(&pos))) {
            if (strListIsMember(&strConnection, e->name, ','))
                delAt(pos, headers_deleted);
        }
        if (headers_deleted)
            refreshMask();
    }
}

