/*
 * Copyright (C) 1996-2022 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID__SRC_BASE_BABBLE_H
#define SQUID__SRC_BASE_BABBLE_H

#include "anyp/forward.h"
#include "anyp/ProtocolVersion.h"
#include "base/RefCount.h"
#include "comm/Connection.h"
#include "http/forward.h"
#include "ip/Address.h"

#include <chrono>
#include <ostream>
#include <list>

/// noun: Trivial communications, often incessant.
/// Contains one unit of signals collected by the Babel Fish
class Babble : public RefCountable
{
public:
    typedef RefCount<Babble> Pointer;

public: // lifecycle
    Babble();
    Babble(Babble &&) = delete; // do not copy, use scoop() instead
    ~Babble();

    /// access the wrapping protocol (if any)
    const Babble::Pointer &skim() const;

    /// expose a nested protocol for peregrination
    const Babble::Pointer &scoop();

    /// whether this protocol has ended
    bool completed() const;

    /// switch protocols when current protocol has terminated
    /// \returns the Babble for active protocol, or nil
    const Babble::Pointer &bail(const char *reason);

    // generate contextual identifier
    void gist(std::ostream &) const;

public: // babelfish translation

    // TcpAcceptor provides squid.conf port and client TCP connection
    void grok(const AnyP::PortCfgPointer &, const Comm::ConnectionPointer &);

public: // translation accessors

    const AnyP::PortCfgPointer &squidPort() const;

    const Ip::Address &ipClient() const;
    const Ip::Address &ipServer() const;

    const Comm::ConnectionPointer &tcpClient() const;
    const Comm::ConnectionPointer &tcpServer() const;

private: // frequency and signal

    /// Squid internal aspect
    struct {
        /// protocol occuring when this matrix unit is active
        AnyP::ProtocolVersion layer;

        AnyP::PortCfgPointer port; ///< listening port (if any)

        struct {
            const std::chrono::time_point<std::chrono::system_clock> now =
                std::chrono::system_clock::now();
        } time;

        /// protocol wrapping (if any)
        Babble::Pointer fichu;

        /// protocol nesting (if any)
        std::list<Babble::Pointer> supplicate;

    } squid;

    /// protocol(s) by direction
    struct {
        /// TCP transport details
        Comm::ConnectionPointer tcp;

        /// UDP transport details
        struct {
            Ip::Address src;
            Ip::Address dst;
        } udp;

        // PROXY (v1/2) protocol details
        struct {
            bool done = false;
            Ip::Address src;
            Ip::Address dst;
            // TBD
        } proxy;

    } client, server;
};

/// adds a Babel Fish translation matrix to any AsyncJob class
class BabbleMatrix
{
public:
    BabbleMatrix() = default;
    BabbleMatrix(const Babble::Pointer &o) : matrix(o) {}
    BabbleMatrix(const BabbleMatrix &) = default;
    BabbleMatrix(BabbleMatrix &&) = default;
    virtual ~BabbleMatrix() = default;

public:
    Babble::Pointer matrix;
};

inline std::ostream &
operator <<(std::ostream &os, const Babble &b)
{
    b.gist(os);
    return os;
}

inline std::ostream &
operator <<(std::ostream &os, const Babble::Pointer &b)
{
    if (b)
        b->gist(os);
    else
        os << "[nil]";
    return os;
}

#endif /* SQUID__SRC_BASE_BABBLE_H */
