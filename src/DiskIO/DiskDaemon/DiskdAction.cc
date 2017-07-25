/*
 * Copyright (C) 1996-2017 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 79    Squid-side DISKD I/O functions. */

#include "squid.h"
#include "base/PackableStream.h"
#include "base/TextException.h"
#include "DiskIO/DiskDaemon/DiskdAction.h"
#include "DiskIO/DiskDaemon/DiskdIOStrategy.h"
#include "ipc/Messages.h"
#include "ipc/TypedMsgHdr.h"
#include "mgr/ActionWriter.h"
#include "SquidString.h"
#include "Store.h"
#include "tools.h"

DiskdAction::Pointer
DiskdAction::Create(const Mgr::CommandPointer &aCmd)
{
    return new DiskdAction(aCmd);
}

DiskdAction::DiskdAction(const Mgr::CommandPointer &aCmd):
    Action(aCmd)
{
    debugs(79, 5, HERE);
}

const char *
DiskdAction::contentType(const String &accepts) const
{
    if (accepts.find("text/yaml") != String::npos) {
        fmtYaml = true;
        return "text/yaml;charset=utf-8";
    }
    return "text/plain;charset=utf-8";
}

void
DiskdAction::add(const Action& action)
{
    debugs(79, 5, HERE);
    data += dynamic_cast<const DiskdAction&>(action).data;
}

void
DiskdAction::collect()
{
    data = diskd_stats;
}

static void
ioTableRow(PackableStream &stream, const char *op, const DiskdStats::iops &data, bool yaml)
{
    if (yaml)
        stream << "  - [ name: " << op << ", ops: " << data.ops <<
               ", success: " << data.success << ", fail: " << data.fail << " ]" << std::endl;
    else
      stream << op << "\t" << data.ops << "\t" << data.success << "\t" << data.fail << std::endl;
}

void
DiskdAction::dump(StoreEntry* entry)
{
    debugs(79, 5, HERE);
    Must(entry);
    PackableStream stream(*entry);

    // this report chunk is identical in both TXT and YAML
    stream << "Counters:" << std::endl <<
           "  sent_count: " << data.sent_count << std::endl <<
           "  recv_count: " << data.recv_count << std::endl <<
           "  max_away: " << data.max_away << std::endl <<
           "  max_shmuse: " << data.max_shmuse << std::endl <<
           "  open_fail_queue_len: " << data.open_fail_queue_len << std::endl <<
           "  block_queue_len: " << data.block_queue_len << std::endl <<
           std::endl;

    stream << "IOPS Table:" << std::endl;
    if (fmtYaml)
        stream << "  - [ name: NAME, ops: OPS, success: SUCCESS, fail: FAIL ]" << std::endl;
    else
        stream << "\tOPS\tSUCCESS\tFAIL" << std::endl;

    ioTableRow(stream, "open", data.open, fmtYaml);
    ioTableRow(stream, "create", data.create, fmtYaml);
    ioTableRow(stream, "close", data.close, fmtYaml);
    ioTableRow(stream, "unlink", data.unlink, fmtYaml);
    ioTableRow(stream, "read", data.read, fmtYaml);
    ioTableRow(stream, "write", data.write, fmtYaml);
    stream.flush();
}

void
DiskdAction::pack(Ipc::TypedMsgHdr& hdrMsg) const
{
    hdrMsg.setType(Ipc::mtCacheMgrResponse);
    hdrMsg.putPod(data);
}

void
DiskdAction::unpack(const Ipc::TypedMsgHdr& hdrMsg)
{
    hdrMsg.checkType(Ipc::mtCacheMgrResponse);
    hdrMsg.getPod(data);
}

