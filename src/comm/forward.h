/*
 * Copyright (C) 1996-2018 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef _SQUID_COMM_FORWARD_H
#define _SQUID_COMM_FORWARD_H

#include "base/RefCount.h"
#include "sbuf/forward.h"

#include <vector>

/// legacy CBDATA callback functions ABI definition for read or write I/O events
/// \deprecated use CommCalls API instead where possible
typedef void PF(int, void *);

/// Abstraction layer for TCP, UDP, TLS, UDS and filedescriptor sockets.
namespace Comm
{

class Connection;
class ConnOpener;

typedef RefCount<Comm::Connection> ConnectionPointer;

typedef std::vector<Comm::ConnectionPointer> ConnectionList;

class IdleConnList;

bool IsConnOpen(const Comm::ConnectionPointer &conn);

// callback handler to process an FD which is available for writing.
PF HandleWrite;

typedef const SBuf PconnKey;
class PconnModule;
class PconnPool;

/// Mark an FD to be watched for its IO status.
void SetSelect(int, unsigned int, PF *, void *, time_t);

}; // namespace Comm

#endif /* _SQUID_COMM_FORWARD_H */

