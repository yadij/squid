/*
 * Copyright (C) 1996-2023 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "base/PackableStream.h"
#include "ipc/Messages.h"
#include "ipc/TypedMsgHdr.h"
#include "mgr/Registration.h"
#include "sbuf/Stats.h"
#include "SBufStatsAction.h"

/// creates a new size-at-destruct-time histogram
static StatHist *
makeDestructTimeHist() {
    const auto stats = new StatHist;
    stats->logInit(100, 30.0, 128000.0);
    return stats;
}

/// the SBuf size-at-destruct-time histogram
static StatHist &
collectSBufDestructTimeStats()
{
    static auto stats = makeDestructTimeHist();
    return *stats;
}

/// the MemBlob size-at-destruct-time histogram
static StatHist &
collectMemBlobDestructTimeStats()
{
    static auto stats = makeDestructTimeHist();
    return *stats;
}

/// record the size an SBuf had when it was destructed
static void
recordSBufSizeAtDestruct(const size_t sz)
{
    collectSBufDestructTimeStats().count(static_cast<double>(sz));
}

/// record the size a MemBlob had when it was destructed
static void
recordMemBlobSizeAtDestruct(const size_t sz)
{
    collectMemBlobDestructTimeStats().count(static_cast<double>(sz));
}

SBufStatsAction::SBufStatsAction(const Mgr::CommandPointer &cmd_):
    Action(cmd_)
{ } //default constructor is OK for data member

SBufStatsAction::Pointer
SBufStatsAction::Create(const Mgr::CommandPointer &cmd)
{
    return new SBufStatsAction(cmd);
}

void
SBufStatsAction::add(const Mgr::Action& action)
{
    sbdata += dynamic_cast<const SBufStatsAction&>(action).sbdata;
    mbdata += dynamic_cast<const SBufStatsAction&>(action).mbdata;
    sbsizesatdestruct += dynamic_cast<const SBufStatsAction&>(action).sbsizesatdestruct;
    mbsizesatdestruct += dynamic_cast<const SBufStatsAction&>(action).mbsizesatdestruct;
}

void
SBufStatsAction::collect()
{
    sbdata = SBuf::GetStats();
    mbdata = MemBlob::GetStats();
    sbsizesatdestruct = collectSBufDestructTimeStats();
    mbsizesatdestruct = collectMemBlobDestructTimeStats();
}

void
SBufStatsAction::dump(StoreEntry* entry)
{
    PackableStream os(*entry);
    os << "\n\n\nThese statistics are experimental; their format and contents "
       "should not be relied upon, they are bound to change as "
       "the SBuf feature is evolved\n";
    sbdata.dump(os);
    mbdata.dump(os);
    os << '\n';

    // display a histogram table row in legacy cachemgr text/plain syntax
    //   table-row = SP 1*VCHAR 1*( HTAB 0*VCHAR )
    const auto tableRow = [&os](int, double val, double size, int count){
        if (count) {
            os << ' ' << val << '-' << (val+size)
               << '\t' << count
               << '\n';
        }
    };

    os << "SBuf size distribution at destruct time:\n";
    sbsizesatdestruct.display(tableRow);

    os << "MemBlob capacity distribution at destruct time:\n";
    mbsizesatdestruct.display(tableRow);
}

void
SBufStatsAction::pack(Ipc::TypedMsgHdr& msg) const
{
    msg.setType(Ipc::mtCacheMgrResponse);
    msg.putPod(sbdata);
    msg.putPod(mbdata);
}

void
SBufStatsAction::unpack(const Ipc::TypedMsgHdr& msg)
{
    msg.checkType(Ipc::mtCacheMgrResponse);
    msg.getPod(sbdata);
    msg.getPod(mbdata);
}

void
SBufStatsAction::RegisterWithCacheManager()
{
    SBufStats::MemBlobSizeAtDestructRecorder = &recordMemBlobSizeAtDestruct;
    SBufStats::SBufSizeAtDestructRecorder = &recordSBufSizeAtDestruct;
    Mgr::RegisterAction("sbuf", "String-Buffer statistics", &SBufStatsAction::Create, 0, 1);
}

