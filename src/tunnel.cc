/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 26    Secure Sockets Layer Proxy */

#include "squid.h"
#include "acl/FilledChecklist.h"
#include "base/AsyncCallbacks.h"
#include "base/CbcPointer.h"
#include "base/JobWait.h"
#include "base/Raw.h"
#include "CachePeer.h"
#include "cbdata.h"
#include "client_side.h"
#include "client_side_request.h"
#include "clients/HttpTunneler.h"
#include "comm.h"
#include "comm/Connection.h"
#include "comm/ConnOpener.h"
#include "comm/Read.h"
#include "comm/Write.h"
#include "errorpage.h"
#include "fd.h"
#include "fde.h"
#include "FwdState.h"
#include "globals.h"
#include "HappyConnOpener.h"
#include "http.h"
#include "http/StatusCode.h"
#include "http/Stream.h"
#include "HttpRequest.h"
#include "icmp/net_db.h"
#include "ip/QosConfig.h"
#include "LogTags.h"
#include "MemBuf.h"
#include "neighbors.h"
#include "PeerSelectState.h"
#include "ResolvedPeers.h"
#include "sbuf/SBuf.h"
#include "security/BlindPeerConnector.h"
#include "SquidConfig.h"
#include "StatCounters.h"
#if USE_OPENSSL
#include "ssl/bio.h"
#include "ssl/ServerBump.h"
#endif
#include "tools.h"
#include "tunnel.h"
#if USE_DELAY_POOLS
#include "DelayId.h"
#endif

#include <climits>
#include <cerrno>

/**
 * TunnelStateData is the state engine performing the tasks for
 * setup of a TCP tunnel from an existing open client FD to a server
 * then shuffling binary data between the resulting FD pair.
 */
/*
 * TODO 1: implement a read/write API on ConnStateData to send/receive blocks
 * of pre-formatted data. Then we can use that as the client side of the tunnel
 * instead of re-implementing it here and occasionally getting the ConnStateData
 * read/write state wrong.
 *
 * TODO 2: then convert this into a AsyncJob, possibly a child of 'Server'
 */
class TunnelStateData: public PeerSelectionInitiator
{
    CBDATA_CHILD(TunnelStateData);

public:
    TunnelStateData(ClientHttpRequest *);
    ~TunnelStateData() override;
    TunnelStateData(const TunnelStateData &); // do not implement
    TunnelStateData &operator =(const TunnelStateData &); // do not implement

    class Connection;
    static void ReadClient(const Comm::ConnectionPointer &, char *buf, size_t len, Comm::Flag errcode, int xerrno, void *data);
    static void ReadServer(const Comm::ConnectionPointer &, char *buf, size_t len, Comm::Flag errcode, int xerrno, void *data);
    static void WriteClientDone(const Comm::ConnectionPointer &, char *buf, size_t len, Comm::Flag flag, int xerrno, void *data);
    static void WriteServerDone(const Comm::ConnectionPointer &, char *buf, size_t len, Comm::Flag flag, int xerrno, void *data);

    bool noConnections() const;
    /// closes both client and server connections
    void closeConnections();

    char *url;
    CbcPointer<ClientHttpRequest> http;
    HttpRequest::Pointer request;
    AccessLogEntryPointer al;

    const char * getHost() const {
        return (server.conn != nullptr && server.conn->getPeer() ? server.conn->getPeer()->host : request->url.host());
    };

    /// store the given to-server connection; prohibit retries and do not look
    /// for any other destinations
    void commitToServer(const Comm::ConnectionPointer &);

    /// Whether the client sent a CONNECT request to us.
    bool clientExpectsConnectResponse() const {
        // If we are forcing a tunnel after receiving a client CONNECT, then we
        // have already responded to that CONNECT before tunnel.cc started.
        if (request && request->flags.forceTunnel)
            return false;
#if USE_OPENSSL
        // We are bumping and we had already send "OK CONNECTED"
        if (http.valid() && http->getConn() && http->getConn()->serverBump() && http->getConn()->serverBump()->at(XactionStep::tlsBump2, XactionStep::tlsBump3))
            return false;
#endif
        return !(request != nullptr &&
                 (request->flags.interceptTproxy || request->flags.intercepted));
    }

    /// starts connecting to the next hop, either for the first time or while
    /// recovering from the previous connect failure
    void startConnecting();
    void closePendingConnection(const Comm::ConnectionPointer &conn, const char *reason);

    /// called when negotiations with the peer have been successfully completed
    void notePeerReadyToShovel(const Comm::ConnectionPointer &);

    class Connection
    {

    public:
        Connection() : len (0), buf ((char *)xmalloc(SQUID_TCP_SO_RCVBUF)), size_ptr(nullptr), delayedLoops(0),
            dirty(false),
            readPending(nullptr), readPendingFunc(nullptr) {}

        ~Connection();

        /// initiates Comm::Connection ownership, including closure monitoring
        template <typename Method>
        void initConnection(const Comm::ConnectionPointer &aConn, Method method, const char *name, TunnelStateData *tunnelState);

        /// reacts to the external closure of our connection
        void noteClosure();

        int bytesWanted(int lower=0, int upper = INT_MAX) const;
        void bytesIn(int const &);
#if USE_DELAY_POOLS

        void setDelayId(DelayId const &);
#endif

        void error(int const xerrno);
        int debugLevelForError(int const xerrno) const;

        void dataSent (size_t amount);
        /// writes 'b' buffer, setting the 'writer' member to 'callback'.
        void write(const char *b, int size, AsyncCall::Pointer &callback, FREE * free_func);
        int len;
        char *buf;
        AsyncCall::Pointer writer; ///< pending Comm::Write callback
        uint64_t *size_ptr;      /* pointer to size in an ConnStateData for logging */

        Comm::ConnectionPointer conn;    ///< The currently connected connection.
        uint8_t delayedLoops; ///< how many times a read on this connection has been postponed.

        bool dirty; ///< whether write() has been called (at least once)

        // XXX: make these an AsyncCall when event API can handle them
        TunnelStateData *readPending;
        EVH *readPendingFunc;

#if USE_DELAY_POOLS

        DelayId delayId;
#endif

    private:
        /// the registered close handler for the connection
        AsyncCall::Pointer closer;
    };

    Connection client, server;
    int *status_ptr;        ///< pointer for logging HTTP status

    SBuf preReadClientData;
    SBuf preReadServerData;
    time_t startTime; ///< object creation time, before any peer selection/connection attempts
    ResolvedPeersPointer destinations; ///< paths for forwarding the request
    bool destinationsFound; ///< At least one candidate path found

    /// whether the decision to tunnel to a particular destination was final
    bool committedToServer;

    int n_tries; ///< the number of forwarding attempts so far

    /// a reason to ban reforwarding attempts (or nil)
    const char *banRetries;

    // TODO: remove after fixing deferred reads in TunnelStateData::copyRead()
    CodeContext::Pointer codeContext; ///< our creator context

    /// waits for a transport connection to the peer to be established/opened
    JobWait<HappyConnOpener> transportWait;

    /// waits for the established transport connection to be secured/encrypted
    JobWait<Security::PeerConnector> encryptionWait;

    /// waits for an HTTP CONNECT tunnel through a cache_peer to be negotiated
    /// over the (encrypted, if needed) transport connection to that cache_peer
    JobWait<Http::Tunneler> peerWait;

    /// Measures time spent on selecting and communicating with peers.
    PeeringActivityTimer peeringTimer;

    void copyRead(Connection &from, IOCB *completion);

    /// continue to set up connection to a peer, going async for SSL peers
    void connectToPeer(const Comm::ConnectionPointer &);
    void secureConnectionToPeer(const Comm::ConnectionPointer &);

    /* PeerSelectionInitiator API */
    void noteDestination(Comm::ConnectionPointer conn) override;
    void noteDestinationsEnd(ErrorState *selectionError) override;

    void syncHierNote(const Comm::ConnectionPointer &server, const char *origin);

    /// called when a connection has been successfully established or
    /// when all candidate destinations have been tried and all have failed
    void noteConnection(HappyConnOpenerAnswer &);

    /// Start using an established connection
    void connectDone(const Comm::ConnectionPointer &conn, const char *origin, const bool reused);

    void notifyConnOpener();

    void saveError(ErrorState *finalError);
    void sendError(ErrorState *finalError, const char *reason);

private:
    void usePinned();

    /// callback handler for the Security::PeerConnector encryptor
    void noteSecurityPeerConnectorAnswer(Security::EncryptorAnswer &);

    /// called after connection setup (including any encryption)
    void connectedToPeer(const Comm::ConnectionPointer &);
    void establishTunnelThruProxy(const Comm::ConnectionPointer &);

    template <typename StepStart>
    void advanceDestination(const char *stepDescription, const Comm::ConnectionPointer &conn, const StepStart &startStep);

    /// \returns whether the request should be retried (nil) or the description why it should not
    const char *checkRetry();

    bool transporting() const;

    // TODO: convert to unique_ptr
    /// details of the "last tunneling attempt" failure (if it failed)
    ErrorState *savedError = nullptr;

    /// resumes operations after the (possibly failed) HTTP CONNECT exchange
    void tunnelEstablishmentDone(Http::TunnelerAnswer &answer);

    void finishWritingAndDelete(Connection &);
    void deleteThis();

    void cancelStep(const char *reason);

    bool exhaustedTries() const;
    void updateAttempts(int);

public:
    bool keepGoingAfterRead(size_t len, Comm::Flag errcode, int xerrno, Connection &from, Connection &to);
    void copy(size_t len, Connection &from, Connection &to, IOCB *);
    void readServer(char *buf, size_t len, Comm::Flag errcode, int xerrno);
    void readClient(char *buf, size_t len, Comm::Flag errcode, int xerrno);
    void writeClientDone(char *buf, size_t len, Comm::Flag flag, int xerrno);
    void writeServerDone(char *buf, size_t len, Comm::Flag flag, int xerrno);

    void copyClientBytes();
    void copyServerBytes();

    /// handles client-to-Squid connection closure; may destroy us
    void clientClosed();

    /// handles Squid-to-server connection closure; may destroy us
    void serverClosed();

    /// tries connecting to another destination, if available,
    /// otherwise, initiates the transaction termination
    void retryOrBail(const char *context);
};

static ERCB tunnelErrorComplete;
static CLCB tunnelServerClosed;
static CLCB tunnelClientClosed;
static CTCB tunnelTimeout;
static EVH tunnelDelayedClientRead;
static EVH tunnelDelayedServerRead;

/// TunnelStateData::serverClosed() wrapper
static void
tunnelServerClosed(const CommCloseCbParams &params)
{
    const auto tunnelState = reinterpret_cast<TunnelStateData *>(params.data);
    tunnelState->serverClosed();
}

void
TunnelStateData::serverClosed()
{
    server.noteClosure();

    peeringTimer.stop();

    finishWritingAndDelete(client);
}

/// TunnelStateData::clientClosed() wrapper
static void
tunnelClientClosed(const CommCloseCbParams &params)
{
    const auto tunnelState = reinterpret_cast<TunnelStateData *>(params.data);
    tunnelState->clientClosed();
}

void
TunnelStateData::clientClosed()
{
    client.noteClosure();
    finishWritingAndDelete(server);
}

/// Gracefully handles non-retriable connection closure. If necessary, either
/// starts closing the given connection or waits for its pending write to
/// finish. Otherwise, immediately destroys the tunnel object.
/// \prec The other Connection is not open.
void
TunnelStateData::finishWritingAndDelete(Connection &remainingConnection)
{
    if (noConnections())
        return deleteThis();

    // XXX: The code below should precede the noConnections() check above. When
    // there is no writer, we should trigger an immediate noConnections()
    // outcome instead of waiting for an asynchronous call to our own closure
    // callback (that will call this method again). We should not move this code
    // until a noConnections() outcome guarantees a nil writer because such a
    // move will unnecessary delay deleteThis().

    if (remainingConnection.writer) {
        debugs(26, 5, "waiting to finish writing to " << remainingConnection.conn);
        // the write completion callback must close its remainingConnection
        // after noticing that the other connection is gone
        return;
    }

    // XXX: Stop abusing connection closure callback for terminating tunneling
    // in cases like this, where our code knows that tunneling must end. The
    // closure callback should be dedicated to handling rare connection closures
    // originated _outside_ of TunnelStateData (e.g., during shutdown). In all
    // other cases, our own close()-calling code must detail the
    // closure-triggering error (if any) _and_ clear all callbacks: Our code
    // does not need to be (asynchronously) notified of the closure that it
    // itself has initiated! Until that (significant) refactoring,
    // serverClosed() and clientClosed() callbacks will continue to mishandle
    // those rare closures as regular ones, and access.log records will continue
    // to lack some tunneling error indicators/details.
    //
    // This asynchronous close() leads to another finishWritingAndDelete() call
    // but with true noConnections() that finally triggers deleteThis().
    remainingConnection.conn->close();
}

/// destroys the tunnel (after performing potentially-throwing cleanup)
void
TunnelStateData::deleteThis()
{
    assert(noConnections());
    // ConnStateData pipeline should contain the CONNECT we are performing
    // but it may be invalid already (bug 4392)
    if (const auto h = http.valid()) {
        if (const auto c = h->getConn())
            if (const auto ctx = c->pipeline.front())
                ctx->finished();
    }
    delete this;
}

// TODO: Replace with a reusable API guaranteeing non-nil pointer forwarding.
/// safely extracts HttpRequest from a never-nil ClientHttpRequest pointer
static auto &
guaranteedRequest(const ClientHttpRequest * const cr)
{
    Assure(cr);
    Assure(cr->request);
    return *cr->request;
}

TunnelStateData::TunnelStateData(ClientHttpRequest *clientRequest) :
    startTime(squid_curtime),
    destinations(new ResolvedPeers()),
    destinationsFound(false),
    committedToServer(false),
    n_tries(0),
    banRetries(nullptr),
    codeContext(CodeContext::Current()),
    peeringTimer(&guaranteedRequest(clientRequest))
{
    debugs(26, 3, "TunnelStateData constructed this=" << this);
    client.readPendingFunc = &tunnelDelayedClientRead;
    server.readPendingFunc = &tunnelDelayedServerRead;

    assert(clientRequest);
    url = xstrdup(clientRequest->uri);
    request = clientRequest->request;
    Must(request);
    server.size_ptr = &clientRequest->out.size;
    client.size_ptr = &clientRequest->al->http.clientRequestSz.payloadData;
    status_ptr = &clientRequest->al->http.code;
    al = clientRequest->al;
    http = clientRequest;

    al->cache.code.update(LOG_TCP_TUNNEL);

    client.initConnection(clientRequest->getConn()->clientConnection, tunnelClientClosed, "tunnelClientClosed", this);

    AsyncCall::Pointer timeoutCall = commCbCall(5, 4, "tunnelTimeout",
                                     CommTimeoutCbPtrFun(tunnelTimeout, this));
    commSetConnTimeout(client.conn, Config.Timeout.lifetime, timeoutCall);
}

TunnelStateData::~TunnelStateData()
{
    debugs(26, 3, "TunnelStateData destructed this=" << this);
    assert(noConnections());
    xfree(url);
    cancelStep("~TunnelStateData");
    delete savedError;
}

TunnelStateData::Connection::~Connection()
{
    if (readPending)
        eventDelete(readPendingFunc, readPending);

    safe_free(buf);
}

const char *
TunnelStateData::checkRetry()
{
    if (shutting_down)
        return "shutting down";
    if (exhaustedTries())
        return "exhausted tries";
    if (!FwdState::EnoughTimeToReForward(startTime))
        return "forwarding timeout";
    if (banRetries)
        return banRetries;
    if (noConnections())
        return "no connections";

    // TODO: Use std::optional for peer_reply_status to avoid treating zero value specially.
    if (request->hier.peer_reply_status != Http::scNone && !Http::IsReforwardableStatus(request->hier.peer_reply_status))
        return "received HTTP status code is not reforwardable";

    // TODO: check pinned connections; see FwdState::pinnedCanRetry()
    return nullptr;
}

void
TunnelStateData::retryOrBail(const char *context)
{
    assert(!server.conn);

    const auto *bailDescription = checkRetry();
    if (!bailDescription) {
        if (!destinations->empty())
            return startConnecting(); // try connecting to another destination

        if (subscribed) {
            debugs(26, 4, "wait for more destinations to try");
            return; // expect a noteDestination*() call
        }

        // fall through to bail
    }

    /* bail */

    peeringTimer.stop();

    // TODO: Add sendSavedErrorOr(err_type type, Http::StatusCode, context).
    // Then, the remaining method code (below) should become the common part of
    // sendNewError() and sendSavedErrorOr(), used in "error detected" cases.
    if (!savedError)
        saveError(new ErrorState(ERR_CANNOT_FORWARD, Http::scInternalServerError, request.getRaw(), al));
    const auto canSendError = Comm::IsConnOpen(client.conn) && !client.dirty &&
                              clientExpectsConnectResponse();
    if (canSendError)
        return sendError(savedError, bailDescription ? bailDescription : context);
    *status_ptr = savedError->httpStatus;

    finishWritingAndDelete(client);
}

int
TunnelStateData::Connection::bytesWanted(int lowerbound, int upperbound) const
{
#if USE_DELAY_POOLS
    return delayId.bytesWanted(lowerbound, upperbound);
#else
    (void)lowerbound;
    return upperbound;
#endif
}

void
TunnelStateData::Connection::bytesIn(int const &count)
{
    debugs(26, 3, "len=" << len << " + count=" << count);
#if USE_DELAY_POOLS
    delayId.bytesIn(count);
#endif

    len += count;
}

/// update "hierarchy" annotations with a new (possibly failed) destination
/// \param origin the name of the origin server we were trying to reach
void
TunnelStateData::syncHierNote(const Comm::ConnectionPointer &conn, const char *origin)
{
    request->hier.resetPeerNotes(conn, origin);
    al->hier.resetPeerNotes(conn, origin);
}

/// sets n_tries to the given value (while keeping ALE in sync)
void
TunnelStateData::updateAttempts(const int newValue)
{
    Assure(n_tries <= newValue); // n_tries cannot decrease

    // Squid probably creates at most one FwdState/TunnelStateData object per
    // ALE, but, unlike an assignment would, this increment logic works even if
    // Squid uses multiple such objects for a given ALE in some esoteric cases.
    al->requestAttempts += (newValue - n_tries);

    n_tries = newValue;
    debugs(26, 5, n_tries);
}

int
TunnelStateData::Connection::debugLevelForError(int const xerrno) const
{
#ifdef ECONNRESET

    if (xerrno == ECONNRESET)
        return 2;

#endif

    if (ignoreErrno(xerrno))
        return 3;

    return 1;
}

/* Read from server side and queue it for writing to the client */
void
TunnelStateData::ReadServer(const Comm::ConnectionPointer &c, char *buf, size_t len, Comm::Flag errcode, int xerrno, void *data)
{
    TunnelStateData *tunnelState = (TunnelStateData *)data;
    assert(cbdataReferenceValid(tunnelState));
    debugs(26, 3, c);

    tunnelState->readServer(buf, len, errcode, xerrno);
}

void
TunnelStateData::readServer(char *, size_t len, Comm::Flag errcode, int xerrno)
{
    debugs(26, 3, server.conn << ", read " << len << " bytes, err=" << errcode);
    server.delayedLoops=0;

    /*
     * Bail out early on Comm::ERR_CLOSING
     * - close handlers will tidy up for us
     */

    if (errcode == Comm::ERR_CLOSING)
        return;

    if (len > 0) {
        server.bytesIn(len);
        statCounter.server.all.kbytes_in += len;
        statCounter.server.other.kbytes_in += len;
        request->hier.notePeerRead();
    }

    if (keepGoingAfterRead(len, errcode, xerrno, server, client))
        copy(len, server, client, WriteClientDone);
}

void
TunnelStateData::Connection::error(int const xerrno)
{
    debugs(50, debugLevelForError(xerrno), conn << ": read/write failure: " << xstrerr(xerrno));

    if (!ignoreErrno(xerrno))
        conn->close();
}

/* Read from client side and queue it for writing to the server */
void
TunnelStateData::ReadClient(const Comm::ConnectionPointer &, char *buf, size_t len, Comm::Flag errcode, int xerrno, void *data)
{
    TunnelStateData *tunnelState = (TunnelStateData *)data;
    assert (cbdataReferenceValid (tunnelState));

    tunnelState->readClient(buf, len, errcode, xerrno);
}

void
TunnelStateData::readClient(char *, size_t len, Comm::Flag errcode, int xerrno)
{
    debugs(26, 3, client.conn << ", read " << len << " bytes, err=" << errcode);
    client.delayedLoops=0;

    /*
     * Bail out early on Comm::ERR_CLOSING
     * - close handlers will tidy up for us
     */

    if (errcode == Comm::ERR_CLOSING)
        return;

    if (len > 0) {
        client.bytesIn(len);
        statCounter.client_http.kbytes_in += len;
    }

    if (keepGoingAfterRead(len, errcode, xerrno, client, server))
        copy(len, client, server, WriteServerDone);
}

/// Updates state after reading from client or server.
/// Returns whether the caller should use the data just read.
bool
TunnelStateData::keepGoingAfterRead(size_t len, Comm::Flag errcode, int xerrno, Connection &from, Connection &to)
{
    debugs(26, 3, "from={" << from.conn << "}, to={" << to.conn << "}");

    /* I think this is to prevent free-while-in-a-callback behaviour
     * - RBC 20030229
     * from.conn->close() / to.conn->close() done here trigger close callbacks which may free TunnelStateData
     */
    const CbcPointer<TunnelStateData> safetyLock(this);

    /* Bump the source connection read timeout on any activity */
    if (Comm::IsConnOpen(from.conn)) {
        AsyncCall::Pointer timeoutCall = commCbCall(5, 4, "tunnelTimeout",
                                         CommTimeoutCbPtrFun(tunnelTimeout, this));
        commSetConnTimeout(from.conn, Config.Timeout.read, timeoutCall);
    }

    /* Bump the dest connection read timeout on any activity */
    /* see Bug 3659: tunnels can be weird, with very long one-way transfers */
    if (Comm::IsConnOpen(to.conn)) {
        AsyncCall::Pointer timeoutCall = commCbCall(5, 4, "tunnelTimeout",
                                         CommTimeoutCbPtrFun(tunnelTimeout, this));
        commSetConnTimeout(to.conn, Config.Timeout.read, timeoutCall);
    }

    if (errcode)
        from.error (xerrno);
    else if (len == 0 || !Comm::IsConnOpen(to.conn)) {
        debugs(26, 3, "Nothing to write or client gone. Terminate the tunnel.");
        from.conn->close();

        /* Only close the remote end if we've finished queueing data to it */
        if (from.len == 0 && Comm::IsConnOpen(to.conn) ) {
            to.conn->close();
        }
    } else if (cbdataReferenceValid(this)) {
        return true;
    }

    return false;
}

void
TunnelStateData::copy(size_t len, Connection &from, Connection &to, IOCB *completion)
{
    debugs(26, 3, "Schedule Write");
    AsyncCall::Pointer call = commCbCall(5,5, "TunnelBlindCopyWriteHandler",
                                         CommIoCbPtrFun(completion, this));
    to.write(from.buf, len, call, nullptr);
}

/* Writes data from the client buffer to the server side */
void
TunnelStateData::WriteServerDone(const Comm::ConnectionPointer &, char *buf, size_t len, Comm::Flag flag, int xerrno, void *data)
{
    TunnelStateData *tunnelState = (TunnelStateData *)data;
    assert (cbdataReferenceValid (tunnelState));
    tunnelState->server.writer = nullptr;

    tunnelState->writeServerDone(buf, len, flag, xerrno);
}

void
TunnelStateData::writeServerDone(char *, size_t len, Comm::Flag flag, int xerrno)
{
    debugs(26, 3, server.conn << ", " << len << " bytes written, flag=" << flag);

    if (flag == Comm::ERR_CLOSING)
        return;

    request->hier.notePeerWrite();

    /* Error? */
    if (flag != Comm::OK) {
        debugs(26, 4, "to-server write failed: " << xerrno);
        server.error(xerrno); // may call comm_close
        return;
    }

    /* EOF? */
    if (len == 0) {
        debugs(26, 4, "No read input. Closing server connection.");
        server.conn->close();
        return;
    }

    /* Valid data */
    statCounter.server.all.kbytes_out += len;
    statCounter.server.other.kbytes_out += len;
    client.dataSent(len);

    /* If the other end has closed, so should we */
    if (!Comm::IsConnOpen(client.conn)) {
        debugs(26, 4, "Client gone away. Shutting down server connection.");
        server.conn->close();
        return;
    }

    const CbcPointer<TunnelStateData> safetyLock(this); /* ??? should be locked by the caller... */

    if (cbdataReferenceValid(this))
        copyClientBytes();
}

/* Writes data from the server buffer to the client side */
void
TunnelStateData::WriteClientDone(const Comm::ConnectionPointer &, char *buf, size_t len, Comm::Flag flag, int xerrno, void *data)
{
    TunnelStateData *tunnelState = (TunnelStateData *)data;
    assert (cbdataReferenceValid (tunnelState));
    tunnelState->client.writer = nullptr;

    tunnelState->writeClientDone(buf, len, flag, xerrno);
}

void
TunnelStateData::Connection::dataSent(size_t amount)
{
    debugs(26, 3, "len=" << len << " - amount=" << amount);
    assert(amount == (size_t)len);
    len =0;
    /* increment total object size */

    if (size_ptr)
        *size_ptr += amount;

}

void
TunnelStateData::Connection::write(const char *b, int size, AsyncCall::Pointer &callback, FREE * free_func)
{
    writer = callback;
    dirty = true;
    Comm::Write(conn, b, size, callback, free_func);
}

template <typename Method>
void
TunnelStateData::Connection::initConnection(const Comm::ConnectionPointer &aConn, Method method, const char *name, TunnelStateData *tunnelState)
{
    Must(!Comm::IsConnOpen(conn));
    Must(!closer);
    Must(Comm::IsConnOpen(aConn));
    conn = aConn;
    closer = commCbCall(5, 4, name, CommCloseCbPtrFun(method, tunnelState));
    comm_add_close_handler(conn->fd, closer);
}

void
TunnelStateData::Connection::noteClosure()
{
    debugs(26, 3, conn);
    conn = nullptr;
    closer = nullptr;
    writer = nullptr; // may already be nil
}

void
TunnelStateData::writeClientDone(char *, size_t len, Comm::Flag flag, int xerrno)
{
    debugs(26, 3, client.conn << ", " << len << " bytes written, flag=" << flag);

    if (flag == Comm::ERR_CLOSING)
        return;

    /* Error? */
    if (flag != Comm::OK) {
        debugs(26, 4, "from-client read failed: " << xerrno);
        client.error(xerrno); // may call comm_close
        return;
    }

    /* EOF? */
    if (len == 0) {
        debugs(26, 4, "Closing client connection due to 0 byte read.");
        client.conn->close();
        return;
    }

    /* Valid data */
    statCounter.client_http.kbytes_out += len;
    server.dataSent(len);

    /* If the other end has closed, so should we */
    if (!Comm::IsConnOpen(server.conn)) {
        debugs(26, 4, "Server has gone away. Terminating client connection.");
        client.conn->close();
        return;
    }

    CbcPointer<TunnelStateData> safetyLock(this);   /* ??? should be locked by the caller... */

    if (cbdataReferenceValid(this))
        copyServerBytes();
}

static void
tunnelTimeout(const CommTimeoutCbParams &io)
{
    TunnelStateData *tunnelState = static_cast<TunnelStateData *>(io.data);
    debugs(26, 3, io.conn);
    /* Temporary lock to protect our own feets (comm_close -> tunnelClientClosed -> Free) */
    CbcPointer<TunnelStateData> safetyLock(tunnelState);

    tunnelState->closeConnections();
}

void
TunnelStateData::closePendingConnection(const Comm::ConnectionPointer &conn, const char *reason)
{
    debugs(26, 3, "because " << reason << "; " << conn);
    assert(!server.conn);
    if (IsConnOpen(conn))
        conn->close();
}

void
TunnelStateData::closeConnections()
{
    if (Comm::IsConnOpen(server.conn))
        server.conn->close();
    if (Comm::IsConnOpen(client.conn))
        client.conn->close();
}

static void
tunnelDelayedClientRead(void *data)
{
    if (!data)
        return;

    TunnelStateData *tunnel = static_cast<TunnelStateData*>(data);
    const auto savedContext = CodeContext::Current();
    CodeContext::Reset(tunnel->codeContext);
    tunnel->client.readPending = nullptr;
    static uint64_t counter=0;
    debugs(26, 7, "Client read(2) delayed " << ++counter << " times");
    tunnel->copyRead(tunnel->client, TunnelStateData::ReadClient);
    CodeContext::Reset(savedContext);
}

static void
tunnelDelayedServerRead(void *data)
{
    if (!data)
        return;

    TunnelStateData *tunnel = static_cast<TunnelStateData*>(data);
    const auto savedContext = CodeContext::Current();
    CodeContext::Reset(tunnel->codeContext);
    tunnel->server.readPending = nullptr;
    static uint64_t counter=0;
    debugs(26, 7, "Server read(2) delayed " << ++counter << " times");
    tunnel->copyRead(tunnel->server, TunnelStateData::ReadServer);
    CodeContext::Reset(savedContext);
}

void
TunnelStateData::copyRead(Connection &from, IOCB *completion)
{
    assert(from.len == 0);
    // If only the minimum permitted read size is going to be attempted
    // then we schedule an event to try again in a few I/O cycles.
    // Allow at least 1 byte to be read every (0.3*10) seconds.
    int bw = from.bytesWanted(1, SQUID_TCP_SO_RCVBUF);
    // XXX: Delay pools must not delay client-to-Squid traffic (i.e. when
    // from.readPendingFunc is tunnelDelayedClientRead()).
    // XXX: Bug #4913: For delay pools, use delayRead() API instead.
    if (bw == 1 && ++from.delayedLoops < 10) {
        from.readPending = this;
        eventAdd("tunnelDelayedServerRead", from.readPendingFunc, from.readPending, 0.3, true);
        return;
    }

    AsyncCall::Pointer call = commCbCall(5,4, "TunnelBlindCopyReadHandler",
                                         CommIoCbPtrFun(completion, this));
    comm_read(from.conn, from.buf, bw, call);
}

void
TunnelStateData::copyClientBytes()
{
    if (preReadClientData.length()) {
        size_t copyBytes = preReadClientData.length() > SQUID_TCP_SO_RCVBUF ? SQUID_TCP_SO_RCVBUF : preReadClientData.length();
        memcpy(client.buf, preReadClientData.rawContent(), copyBytes);
        preReadClientData.consume(copyBytes);
        client.bytesIn(copyBytes);
        if (keepGoingAfterRead(copyBytes, Comm::OK, 0, client, server))
            copy(copyBytes, client, server, TunnelStateData::WriteServerDone);
    } else
        copyRead(client, ReadClient);
}

void
TunnelStateData::copyServerBytes()
{
    if (preReadServerData.length()) {
        size_t copyBytes = preReadServerData.length() > SQUID_TCP_SO_RCVBUF ? SQUID_TCP_SO_RCVBUF : preReadServerData.length();
        memcpy(server.buf, preReadServerData.rawContent(), copyBytes);
        preReadServerData.consume(copyBytes);
        server.bytesIn(copyBytes);
        if (keepGoingAfterRead(copyBytes, Comm::OK, 0, server, client))
            copy(copyBytes, server, client, TunnelStateData::WriteClientDone);
    } else
        copyRead(server, ReadServer);
}

/**
 * Set the HTTP status for this request and sets the read handlers for client
 * and server side connections.
 */
static void
tunnelStartShoveling(TunnelStateData *tunnelState)
{
    assert(!tunnelState->transportWait);
    assert(!tunnelState->encryptionWait);
    assert(!tunnelState->peerWait);

    assert(tunnelState->server.conn);
    AsyncCall::Pointer timeoutCall = commCbCall(5, 4, "tunnelTimeout",
                                     CommTimeoutCbPtrFun(tunnelTimeout, tunnelState));
    commSetConnTimeout(tunnelState->server.conn, Config.Timeout.read, timeoutCall);

    *tunnelState->status_ptr = Http::scOkay;
    if (cbdataReferenceValid(tunnelState)) {

        // Shovel any payload already pushed into reply buffer by the server response
        if (!tunnelState->server.len)
            tunnelState->copyServerBytes();
        else {
            debugs(26, DBG_DATA, "Tunnel server PUSH Payload: \n" << Raw("", tunnelState->server.buf, tunnelState->server.len) << "\n----------");
            tunnelState->copy(tunnelState->server.len, tunnelState->server, tunnelState->client, TunnelStateData::WriteClientDone);
        }

        if (tunnelState->http.valid() && tunnelState->http->getConn() && !tunnelState->http->getConn()->inBuf.isEmpty()) {
            SBuf * const in = &tunnelState->http->getConn()->inBuf;
            debugs(26, DBG_DATA, "Tunnel client PUSH Payload: \n" << *in << "\n----------");
            tunnelState->preReadClientData.append(*in);
            in->consume(); // ConnStateData buffer accounting after the shuffle.
        }
        tunnelState->copyClientBytes();
    }
}

/**
 * All the pieces we need to write to client and/or server connection
 * have been written.
 * Call the tunnelStartShoveling to start the blind pump.
 */
static void
tunnelConnectedWriteDone(const Comm::ConnectionPointer &conn, char *, size_t len, Comm::Flag flag, int, void *data)
{
    TunnelStateData *tunnelState = (TunnelStateData *)data;
    debugs(26, 3, conn << ", flag=" << flag);
    tunnelState->client.writer = nullptr;

    if (flag != Comm::OK) {
        *tunnelState->status_ptr = Http::scInternalServerError;
        tunnelErrorComplete(conn->fd, data, 0);
        return;
    }

    if (auto http = tunnelState->http.get()) {
        http->out.headers_sz += len;
        http->out.size += len;
    }

    tunnelStartShoveling(tunnelState);
}

void
TunnelStateData::tunnelEstablishmentDone(Http::TunnelerAnswer &answer)
{
    peerWait.finish();
    server.len = 0;

    // XXX: al->http.code (i.e. *status_ptr) should not be (re)set
    // until we actually start responding to the client. Right here/now, we only
    // know how this cache_peer has responded to us.
    if (answer.peerResponseStatus != Http::scNone)
        *status_ptr = answer.peerResponseStatus;

    auto sawProblem = false;

    if (!answer.positive()) {
        sawProblem = true;
        assert(!answer.conn);
    } else if (!Comm::IsConnOpen(answer.conn) || fd_table[answer.conn->fd].closing()) {
        sawProblem = true;
        closePendingConnection(answer.conn, "conn was closed while waiting for tunnelEstablishmentDone");
    }

    if (!sawProblem) {
        assert(answer.positive()); // paranoid
        // copy any post-200 OK bytes to our buffer
        preReadServerData = answer.leftovers;
        notePeerReadyToShovel(answer.conn);
        return;
    }

    ErrorState *error = nullptr;
    if (answer.positive()) {
        error = new ErrorState(ERR_CANNOT_FORWARD, Http::scServiceUnavailable, request.getRaw(), al);
    } else {
        error = answer.squidError.get();
        Must(error);
        answer.squidError.clear(); // preserve error for errorSendComplete()
    }
    assert(error);
    saveError(error);
    retryOrBail("tunneler error");
}

void
TunnelStateData::notePeerReadyToShovel(const Comm::ConnectionPointer &conn)
{
    assert(!client.dirty);
    commitToServer(conn);

    if (!clientExpectsConnectResponse())
        tunnelStartShoveling(this); // ssl-bumped connection, be quiet
    else {
        *status_ptr = Http::scOkay;
        AsyncCall::Pointer call = commCbCall(5,5, "tunnelConnectedWriteDone",
                                             CommIoCbPtrFun(tunnelConnectedWriteDone, this));
        al->reply = HttpReply::MakeConnectionEstablished();
        const auto mb = al->reply->pack();
        client.write(mb->content(), mb->contentSize(), call, mb->freeFunc());
        delete mb;
    }
}

void
TunnelStateData::commitToServer(const Comm::ConnectionPointer &conn)
{
    committedToServer = true;
    banRetries = "committed to server";
    PeerSelectionInitiator::subscribed = false; // may already be false
    server.initConnection(conn, tunnelServerClosed, "tunnelServerClosed", this);
}

static void
tunnelErrorComplete(int fd/*const Comm::ConnectionPointer &*/, void *data, size_t)
{
    TunnelStateData *tunnelState = (TunnelStateData *)data;
    debugs(26, 3, "FD " << fd);
    assert(tunnelState != nullptr);
    /* temporary lock to save our own feets (comm_close -> tunnelClientClosed -> Free) */
    CbcPointer<TunnelStateData> safetyLock(tunnelState);

    if (Comm::IsConnOpen(tunnelState->client.conn))
        tunnelState->client.conn->close();

    if (Comm::IsConnOpen(tunnelState->server.conn))
        tunnelState->server.conn->close();
}

void
TunnelStateData::noteConnection(HappyConnOpener::Answer &answer)
{
    transportWait.finish();

    updateAttempts(answer.n_tries);

    ErrorState *error = nullptr;
    if ((error = answer.error.get())) {
        banRetries = "HappyConnOpener gave up";
        Must(!Comm::IsConnOpen(answer.conn));
        syncHierNote(answer.conn, request->url.host());
        answer.error.clear();
    } else if (!Comm::IsConnOpen(answer.conn) || fd_table[answer.conn->fd].closing()) {
        error = new ErrorState(ERR_CANNOT_FORWARD, Http::scServiceUnavailable, request.getRaw(), al);
        closePendingConnection(answer.conn, "conn was closed while waiting for  noteConnection");
    }

    if (error) {
        saveError(error);
        retryOrBail("tried all destinations");
        return;
    }

    connectDone(answer.conn, request->url.host(), answer.reused);
}

void
TunnelStateData::connectDone(const Comm::ConnectionPointer &conn, const char *origin, const bool reused)
{
    Must(Comm::IsConnOpen(conn));

    if (reused)
        ResetMarkingsToServer(request.getRaw(), *conn);
    // else Comm::ConnOpener already applied proper/current markings

    // TODO: add pconn race state tracking

    syncHierNote(conn, origin);

#if USE_DELAY_POOLS
    /* no point using the delayIsNoDelay stuff since tunnel is nice and simple */
    if (conn->getPeer() && conn->getPeer()->options.no_delay)
        server.setDelayId(DelayId());
#endif

    netdbPingSite(request->url.host());

    bool toOrigin = false; // same semantics as StateFlags::toOrigin
    if (const auto * const peer = conn->getPeer()) {
        request->prepForPeering(*peer);
        toOrigin = peer->options.originserver;
    } else {
        request->prepForDirect();
        toOrigin = true;
    }

    if (!toOrigin)
        connectToPeer(conn);
    else {
        notePeerReadyToShovel(conn);
    }
}

/// whether we have used up all permitted forwarding attempts
bool
TunnelStateData::exhaustedTries() const
{
    return n_tries >= Config.forward_max_tries;
}

void
tunnelStart(ClientHttpRequest * http)
{
    debugs(26, 3, MYNAME);
    /* Create state structure. */
    TunnelStateData *tunnelState = nullptr;
    ErrorState *err = nullptr;
    HttpRequest *request = http->request;
    char *url = http->uri;

    /*
     * client_addr.isNoAddr()  indicates this is an "internal" request
     * from peer_digest.c, asn.c, netdb.c, etc and should always
     * be allowed.  yuck, I know.
     */

    if (Config.accessList.miss && !request->client_addr.isNoAddr()) {
        /*
         * Check if this host is allowed to fetch MISSES from us (miss_access)
         * default is to allow.
         */
        ACLFilledChecklist ch(Config.accessList.miss, request);
        ch.al = http->al;
        ch.src_addr = request->client_addr;
        ch.my_addr = request->my_addr;
        ch.syncAle(request, http->log_uri);
        if (ch.fastCheck().denied()) {
            debugs(26, 4, "MISS access forbidden.");
            http->updateLoggingTags(LOG_TCP_TUNNEL);
            err = new ErrorState(ERR_FORWARDING_DENIED, Http::scForbidden, request, http->al);
            http->al->http.code = Http::scForbidden;
            errorSend(http->getConn()->clientConnection, err);
            return;
        }
    }

    debugs(26, 3, request->method << ' ' << url << ' ' << request->http_ver);
    ++statCounter.server.all.requests;
    ++statCounter.server.other.requests;

    tunnelState = new TunnelStateData(http);
#if USE_DELAY_POOLS
    tunnelState->server.setDelayId(DelayId::DelayClient(http));
#endif
    tunnelState->startSelectingDestinations(request, http->al, nullptr);
}

void
TunnelStateData::connectToPeer(const Comm::ConnectionPointer &conn)
{
    if (const auto p = conn->getPeer()) {
        if (p->secure.encryptTransport)
            return advanceDestination("secure connection to peer", conn, [this,&conn] {
            secureConnectionToPeer(conn);
        });
    }

    connectedToPeer(conn);
}

/// encrypts an established TCP connection to peer
void
TunnelStateData::secureConnectionToPeer(const Comm::ConnectionPointer &conn)
{
    const auto callback = asyncCallback(5, 4, TunnelStateData::noteSecurityPeerConnectorAnswer, this);
    const auto connector = new Security::BlindPeerConnector(request, conn, callback, al);
    encryptionWait.start(connector, callback);
}

/// starts a preparation step for an established connection; retries on failures
template <typename StepStart>
void
TunnelStateData::advanceDestination(const char *stepDescription, const Comm::ConnectionPointer &conn, const StepStart &startStep)
{
    // TODO: Extract destination-specific handling from TunnelStateData so that
    // all the awkward, limited-scope advanceDestination() calls can be replaced
    // with a single simple try/catch,retry block.
    try {
        startStep();
        // now wait for the step callback
    } catch (...) {
        debugs (26, 2, "exception while trying to " << stepDescription << ": " << CurrentException);
        closePendingConnection(conn, "connection preparation exception");
        if (!savedError)
            saveError(new ErrorState(ERR_CANNOT_FORWARD, Http::scInternalServerError, request.getRaw(), al));
        retryOrBail(stepDescription);
    }
}

/// callback handler for the connection encryptor
void
TunnelStateData::noteSecurityPeerConnectorAnswer(Security::EncryptorAnswer &answer)
{
    encryptionWait.finish();

    ErrorState *error = nullptr;
    assert(!answer.tunneled);
    if ((error = answer.error.get())) {
        assert(!answer.conn);
        answer.error.clear();
    } else if (!Comm::IsConnOpen(answer.conn) || fd_table[answer.conn->fd].closing()) {
        error = new ErrorState(ERR_CANNOT_FORWARD, Http::scServiceUnavailable, request.getRaw(), al);
        closePendingConnection(answer.conn, "conn was closed while waiting for noteSecurityPeerConnectorAnswer");
    }

    if (error) {
        saveError(error);
        retryOrBail("TLS peer connection error");
        return;
    }

    connectedToPeer(answer.conn);
}

void
TunnelStateData::connectedToPeer(const Comm::ConnectionPointer &conn)
{
    advanceDestination("establish tunnel through proxy", conn, [this,&conn] {
        establishTunnelThruProxy(conn);
    });
}

void
TunnelStateData::establishTunnelThruProxy(const Comm::ConnectionPointer &conn)
{
    const auto callback = asyncCallback(5, 4, TunnelStateData::tunnelEstablishmentDone, this);
    const auto tunneler = new Http::Tunneler(conn, request, callback, Config.Timeout.lifetime, al);
#if USE_DELAY_POOLS
    tunneler->setDelayId(server.delayId);
#endif
    peerWait.start(tunneler, callback);
}

void
TunnelStateData::noteDestination(Comm::ConnectionPointer path)
{
    destinationsFound = true;

    if (!path) { // decided to use a pinned connection
        // We can call usePinned() without fear of clashing with an earlier
        // forwarding attempt because PINNED must be the first destination.
        assert(destinations->empty());
        usePinned();
        return;
    }

    destinations->addPath(path);

    if (transportWait) {
        assert(!transporting());
        notifyConnOpener();
        return; // and continue to wait for tunnelConnectDone() callback
    }

    if (transporting())
        return; // and continue to receive destinations for backup

    startConnecting();
}

void
TunnelStateData::noteDestinationsEnd(ErrorState *selectionError)
{
    PeerSelectionInitiator::subscribed = false;
    destinations->destinationsFinalized = true;
    if (!destinationsFound) {

        // XXX: Honor clientExpectsConnectResponse() before replying.

        if (selectionError)
            return sendError(selectionError, "path selection has failed");

        // TODO: Merge with FwdState and remove this likely unnecessary check.
        if (savedError)
            return sendError(savedError, "path selection found no paths (with an impossible early error)");

        return sendError(new ErrorState(ERR_CANNOT_FORWARD, Http::scInternalServerError, request.getRaw(), al),
                         "path selection found no paths");
    }
    // else continue to use one of the previously noted destinations;
    // if all of them fail, tunneling as whole will fail
    Must(!selectionError); // finding at least one path means selection succeeded

    if (transportWait) {
        assert(!transporting());
        notifyConnOpener();
        return; // and continue to wait for the noteConnection() callback
    }

    if (transporting()) {
        // We are already using a previously opened connection (but were also
        // receiving more destinations in case we need to re-forward).
        debugs(17, 7, "keep transporting");
        return;
    }

    // destinationsFound, but none of them worked, and we were waiting for more
    debugs(17, 7, "no more destinations to try after " << n_tries << " failed attempts");
    if (!savedError) {
        // retryOrBail() must be preceded by saveError(), but in case we forgot:
        const auto finalError = new ErrorState(ERR_CANNOT_FORWARD, Http::scBadGateway, request.getRaw(), al);
        static const auto d = MakeNamedErrorDetail("RETRY_TO_NONE");
        finalError->detailError(d);
        saveError(finalError);
    } // else use actual error from last forwarding attempt

    // XXX: Honor clientExpectsConnectResponse() before replying.
    sendError(savedError, "all found paths have failed");
}

/// Whether a tunneling attempt to some selected destination X is in progress
/// (after successfully opening/reusing a transport connection to X).
/// \sa transportWait
bool
TunnelStateData::transporting() const
{
    return encryptionWait || peerWait || committedToServer;
}

/// remembers an error to be used if there will be no more connection attempts
void
TunnelStateData::saveError(ErrorState *error)
{
    debugs(26, 4, savedError << " ? " << error);
    assert(error);
    delete savedError; // may be nil
    savedError = error;
}

/// Starts sending the given error message to the client, leading to the
/// eventual transaction termination. Call with savedError to send savedError.
void
TunnelStateData::sendError(ErrorState *finalError, const char *reason)
{
    debugs(26, 3, "aborting transaction for " << reason);

    peeringTimer.stop();

    cancelStep(reason);

    assert(finalError);

    // get rid of any cached error unless that is what the caller is sending
    if (savedError != finalError)
        delete savedError; // may be nil
    savedError = nullptr;

    // we cannot try other destinations after responding with an error
    PeerSelectionInitiator::subscribed = false; // may already be false

    *status_ptr = finalError->httpStatus;
    finalError->callback = tunnelErrorComplete;
    finalError->callback_data = this;
    errorSend(client.conn, finalError);
}

/// Notify a pending subtask, if any, that we no longer need its help. We do not
/// have to do this -- the subtask job will eventually end -- but ending it
/// earlier reduces waste and may reduce DoS attack surface.
void
TunnelStateData::cancelStep(const char *reason)
{
    transportWait.cancel(reason);
    encryptionWait.cancel(reason);
    peerWait.cancel(reason);
}

void
TunnelStateData::startConnecting()
{
    assert(!destinations->empty());
    assert(!transporting());

    delete savedError; // may still be nil
    savedError = nullptr;
    request->hier.peer_reply_status = Http::scNone;

    const auto callback = asyncCallback(17, 5, TunnelStateData::noteConnection, this);
    const auto cs = new HappyConnOpener(destinations, callback, request, startTime, n_tries, al);
    cs->setHost(request->url.host());
    cs->setRetriable(false);
    cs->allowPersistent(false);
    destinations->notificationPending = true; // start() is async
    transportWait.start(cs, callback);
}

/// send request on an existing connection dedicated to the requesting client
void
TunnelStateData::usePinned()
{
    Must(request);
    const auto connManager = request->pinnedConnection();
    Comm::ConnectionPointer serverConn = nullptr;

    try {
        serverConn = ConnStateData::BorrowPinnedConnection(request.getRaw(), al);
        debugs(26, 7, "pinned peer connection: " << serverConn);
    } catch (ErrorState * const error) {
        syncHierNote(nullptr, connManager ? connManager->pinning.host : request->url.host());
        // XXX: Honor clientExpectsConnectResponse() before replying.
        // a PINNED path failure is fatal; do not wait for more paths
        sendError(error, "pinned path failure");
        return;
    }

    updateAttempts(n_tries + 1);

    // Set HttpRequest pinned related flags for consistency even if
    // they are not really used by tunnel.cc code.
    request->flags.pinned = true;

    Assure(connManager);
    if (connManager->pinnedAuth())
        request->flags.auth = true;

    // the server may close the pinned connection before this request
    const auto reused = true;
    connectDone(serverConn, connManager->pinning.host, reused);
}

CBDATA_CLASS_INIT(TunnelStateData);

bool
TunnelStateData::noConnections() const
{
    return !Comm::IsConnOpen(server.conn) && !Comm::IsConnOpen(client.conn);
}

#if USE_DELAY_POOLS
void
TunnelStateData::Connection::setDelayId(DelayId const &newDelay)
{
    delayId = newDelay;
}

#endif

/// makes sure connection opener knows that the destinations have changed
void
TunnelStateData::notifyConnOpener()
{
    if (destinations->notificationPending) {
        debugs(17, 7, "reusing pending notification");
    } else {
        destinations->notificationPending = true;
        CallJobHere(17, 5, transportWait.job(), HappyConnOpener, noteCandidatesChange);
    }
}

/**
 * Sets up a TCP tunnel through Squid and starts shoveling traffic.
 * \param request the request that initiated/caused this tunnel
 * \param clientConn the already accepted client-to-Squid TCP connection
 * \param srvConn the already established Squid-to-server TCP connection
 * \param preReadServerData server-sent bytes to be forwarded to the client
 */
void
switchToTunnel(HttpRequest *request, const Comm::ConnectionPointer &clientConn, const Comm::ConnectionPointer &srvConn, const SBuf &preReadServerData)
{
    Must(Comm::IsConnOpen(clientConn));
    Must(Comm::IsConnOpen(srvConn));

    debugs(26,5, "Revert to tunnel FD " << clientConn->fd << " with FD " << srvConn->fd);

    /* Create state structure. */
    ++statCounter.server.all.requests;
    ++statCounter.server.other.requests;

    auto conn = request->clientConnectionManager.get();
    Must(conn);
    Http::StreamPointer context = conn->pipeline.front();
    Must(context && context->http);

    debugs(26, 3, request->method << " " << context->http->uri << " " << request->http_ver);

    TunnelStateData *tunnelState = new TunnelStateData(context->http);
    tunnelState->commitToServer(srvConn);

    request->hier.resetPeerNotes(srvConn, tunnelState->getHost());

#if USE_DELAY_POOLS
    /* no point using the delayIsNoDelay stuff since tunnel is nice and simple */
    if (!srvConn->getPeer() || !srvConn->getPeer()->options.no_delay)
        tunnelState->server.setDelayId(DelayId::DelayClient(context->http));
#endif

    debugs(26, 4, "determine post-connect handling pathway.");
    if (const auto peer = srvConn->getPeer())
        request->prepForPeering(*peer);
    else
        request->prepForDirect();

    tunnelState->preReadServerData = preReadServerData;

    tunnelStartShoveling(tunnelState);
}

