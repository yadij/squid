
#include "squid.h"
#if USE_DELAY_POOLS

#include "ClientInfo.h"
#include "fde.h"
#include "shaping/QuotaQueue.h"

CBDATA_NAMESPACED_CLASS_INIT(Shaping, QuotaQueue);

Shaping::QuotaQueue::QuotaQueue(ClientInfo *info)
    : clientInfo(info)
{
    assert(clientInfo);
}

Shaping::QuotaQueue::~QuotaQueue()
{
    assert(!clientInfo); // ClientInfo should clear this before destroying us
}

/// places the given fd at the end of the queue; returns reservation ID
unsigned int
Shaping::QuotaQueue::enqueue(int fd)
{
    debugs(77, 5, "clt" << (const char*)clientInfo->key <<
           ": FD " << fd << " with qqid" << (ins+1) << ' ' << fds.size());
    fds.push_back(fd);
    fd_table[fd].codeContext = CodeContext::Current();
    return ++ins;
}

/// removes queue head
void
Shaping::QuotaQueue::dequeue()
{
    assert(!fds.empty());
    debugs(77, 5, "clt" << (const char*)clientInfo->key <<
           ": FD " << fds.front() << " with qqid" << (outs+1) << ' ' <<
           fds.size());
    fds.pop_front();
    ++outs;
}

#endif /* USE_DELAY_POOLS */
