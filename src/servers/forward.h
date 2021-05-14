/*
 * Copyright (C) 1996-2021 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_SERVERS_FORWARD_H
#define SQUID_SERVERS_FORWARD_H

#include "base/forward.h"

class ConnStateData;

namespace Http
{

namespace One
{
class Server;
} // namespace One

/// create a new HTTP connection handler; never returns NULL
ConnStateData *NewServer(const Squid::XactPointer &);

} // namespace Http

namespace Https
{

/// create a new HTTPS connection handler; never returns NULL
ConnStateData *NewServer(const Squid::XactPointer &);

} // namespace Https

namespace Ftp
{

/// accept connections on all configured ftp_ports
void StartListening();
/// reject new connections to any configured ftp_port
void StopListening();

} // namespace Ftp

#endif /* SQUID_SERVERS_FORWARD_H */

