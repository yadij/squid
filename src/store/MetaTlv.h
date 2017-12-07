/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID__STORE_METATLV_H
#define SQUID__STORE_METATLV_H

#include "store/forward.h"

/**
 \section StoreSwapMeta Store "swap meta" Description
 \par
 * "swap meta" refers to a section of meta data stored at the beginning
 * of an object that is stored on disk.  This meta data includes information
 * such as the object's cache key (MD5), URL, and part of the StoreEntry
 * structure.
 *
 \par
 * The meta data is stored using a TYPE-LENGTH-VALUE format.  That is,
 * each chunk of meta information consists of a TYPE identifier, a
 * LENGTH field, and then the VALUE (which is LENGTH octets long).
 *
 \par
 * \note Type numbers are stored to disk files and not re-usable.
 *       Each type which is enumerated must have a unique number assigned
 *       to reserve that value permanently.
 */
enum {
    /**
     * Just a placeholder for the zeroth value. It is never used on disk.
     */
    STORE_META_VOID = 0,

    /**
     \deprecated
     * This represents the case when we use the URL as the cache
     * key, as Squid-1.1 does.  Currently we don't support using
     * a URL as a cache key, so this is not used.
     */
    STORE_META_KEY_URL = 1,

    /**
     \deprecated
     * For a brief time we considered supporting SHA (secure
     * hash algorithm) as a cache key.  Nobody liked it, and
     * this type is not currently used.
     */
    STORE_META_KEY_SHA = 2,

    /**
     * This represents the MD5 cache key that Squid currently uses.
     * When Squid opens a disk file for reading, it can check that
     * this MD5 matches the MD5 of the user's request.  If not, then
     * something went wrong and this is probably the wrong object.
     */
    STORE_META_KEY_MD5 = 3,

    /**
     * The object's URL.  This also may be matched against a user's
     *  request for cache hits to make sure we got the right object.
     */
    STORE_META_URL = 4,

    /**
     * This is the "standard metadata" for an object.
     * Really its just this middle chunk of the StoreEntry structure:
     \code
        time_t timestamp;
        time_t lastref;
        time_t expires;
        time_t lastmod;
        uint64_t swap_file_sz;
        uint16_t refcount;
        uint16_t flags;
     \endcode
     */
    STORE_META_STD = 5,

    /**
     * Reserved for future hit-metering (RFC 2227) stuff
     */
    STORE_META_HITMETERING = 6,

    /// \todo DOCS: document.
    STORE_META_VALID = 7,

    /**
     * Stores Vary request headers
     */
    STORE_META_VARY_HEADERS = 8,

    /**
     * Updated version of STORE_META_STD, with support for  >2GB objects.
     * As STORE_META_STD except that the swap_file_sz is a 64-bit integer instead of 32-bit.
     */
    STORE_META_STD_LFS = 9,

    STORE_META_OBJSIZE = 10,

    STORE_META_STOREURL = 11,    /* the store url, if different to the normal URL */
    STORE_META_VARY_ID = 12,     /* Unique ID linking variants */
    STORE_META_END
};

namespace Store {

class MetaTlv
{
protected:
    MetaTlv() {}
    MetaTlv(const MetaTlv &) = default;
    MetaTlv& operator=(const MetaTlv &) = default;

public:
    static bool validType(char);
    static int const MaximumTLVLength;
    static int const MinimumTLVLength;
    static Store::MetaTlv *Factory(char type, size_t len, void const *value);
    static Store::MetaTlv **Add(Store::MetaTlv **tail, Store::MetaTlv *aNode);
    static void FreeList(Store::MetaTlv **head);

    virtual char getType() const = 0;
    virtual bool validLength(int) const;
    virtual bool checkConsistency(StoreEntry *) const;
    virtual ~MetaTlv() {}

    int length = -1;
    void *value = nullptr;
    MetaTlv *next = nullptr;
};

} // namespace Store

char *storeSwapMetaPack(Store::MetaTlv * tlv_list, int *length);
Store::MetaTlv *storeSwapMetaBuild(StoreEntry * e);
void storeSwapTLVFree(Store::MetaTlv * n);

#endif /* SQUID__STORE_METATLV_H */

