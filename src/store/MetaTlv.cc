/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 20    Storage Manager Swapfile Metadata */

#include "squid.h"
#include "base/Range.h"
#include "MemObject.h"
#include "Store.h"
#include "store/MetaMd5.h"
#include "store/MetaObjSize.h"
#include "store/MetaStd.h"
#include "store/MetaStdLfs.h"
#include "store/MetaTlv.h"
#include "store/MetaUrl.h"
#include "store/MetaVary.h"

bool
Store::MetaTlv::validType(char type)
{
    /* VOID is reserved, and new types have to be added as classes */
    if (type <= STORE_META_VOID || type >= STORE_META_END + 10) {
        debugs(20, DBG_CRITICAL, "storeSwapMetaUnpack: bad type (" << type << ")!");
        return false;
    }

    /* Not yet implemented */
    if (type >= STORE_META_END ||
            type == STORE_META_STOREURL ||
            type == STORE_META_VARY_ID) {
        debugs(20, 3, "storeSwapMetaUnpack: Not yet implemented (" << type << ") in disk metadata");
        return false;
    }

    /* Unused in any current squid code */
    if (type == STORE_META_KEY_URL ||
            type == STORE_META_KEY_SHA ||
            type == STORE_META_HITMETERING ||
            type == STORE_META_VALID) {
        debugs(20, DBG_CRITICAL, "Obsolete and unused type (" << type << ") in disk metadata");
        return false;
    }

    return true;
}

const int Store::MetaTlv::MinimumTLVLength = 0;
const int Store::MetaTlv::MaximumTLVLength = 1 << 16;

bool
Store::MetaTlv::validLength(int aLength) const
{
    static const Range<int> TlvValidLengths = Range<int>(Store::MetaTlv::MinimumTLVLength, Store::MetaTlv::MaximumTLVLength);
    if (!TlvValidLengths.contains(aLength)) {
        debugs(20, DBG_CRITICAL, MYNAME << ": insane length (" << aLength << ")!");
        return false;
    }

    return true;
}

Store::MetaTlv *
Store::MetaTlv::Factory (char type, size_t len, void const *value)
{
    if (!validType(type))
        return NULL;

    Store::MetaTlv *result;

    switch (type) {

    case STORE_META_KEY:
        result = new StoreMetaMD5;
        break;

    case STORE_META_URL:
        result = new StoreMetaURL;
        break;

    case STORE_META_STD:
        result = new StoreMetaSTD;
        break;

    case STORE_META_STD_LFS:
        result = new StoreMetaSTDLFS;
        break;

    case STORE_META_OBJSIZE:
        result = new StoreMetaObjSize;
        break;

    case STORE_META_VARY_HEADERS:
        result = new StoreMetaVary;
        break;

    default:
        debugs(20, DBG_CRITICAL, "Attempt to create unknown concrete STORE_META");
        return NULL;
    }

    if (!result->validLength(len)) {
        delete result;
        return NULL;
    }

    result->length = len;
    result->value = xmalloc(len);
    memcpy(result->value, value, len);
    return result;
}

void
Store::MetaTlv::FreeList(Store::MetaTlv **head)
{
    while (Store::MetaTlv *node = *head) {
        *head = node->next;
        xfree(node->value);
        delete node;
    }
}

Store::MetaTlv **
Store::MetaTlv::Add(Store::MetaTlv **tail, Store::MetaTlv *aNode)
{
    assert (*tail == NULL);
    *tail = aNode;
    return &aNode->next;        /* return new tail pointer */
}

bool
Store::MetaTlv::checkConsistency(StoreEntry *) const
{
    switch (getType()) {

    case STORE_META_KEY:

    case STORE_META_URL:

    case STORE_META_VARY_HEADERS:
        assert(0);
        break;

    case STORE_META_STD:
        break;

    case STORE_META_STD_LFS:
        break;

    case STORE_META_OBJSIZE:
        break;

    default:
        debugs(20, DBG_IMPORTANT, "WARNING: got unused STORE_META type " << getType());
        break;
    }

    return true;
}
