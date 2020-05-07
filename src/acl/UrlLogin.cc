/*
 * Copyright (C) 1996-2020 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 28    Access Control */

#include "squid.h"
#include "acl/FilledChecklist.h"
#include "acl/RegexData.h"
#include "acl/UrlLogin.h"
#include "HttpRequest.h"
#include "rfc1738.h"

int
ACLUrlLoginStrategy::match(ACLData<char const *> * &data, ACLFilledChecklist *checklist)
{
    const auto &userinfo = checklist->al->request->url.userInfo();
    if (userinfo.isEmpty()) {
        debugs(28, 5, "URL has no user-info details. cannot match");
        return 0; // nothing can match
    }

    static char str[MAX_URL]; // should be big enough for a single URI segment

    const SBuf::size_type len = userinfo.copy(str, sizeof(str)-1);
    str[len] = '\0';
    rfc1738_unescape(str); // XXX: remove, userinfo already stored unescaped
    return data->match(str);
}

