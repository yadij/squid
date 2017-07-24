/*
 * Copyright (C) 1996-2017 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 79    Squid-side DISKD I/O functions. */

#ifndef SQUID_DISKD_ACTION_H
#define SQUID_DISKD_ACTION_H

#include "DiskIO/DiskDaemon/DiskdStats.h"
#include "ipc/forward.h"
#include "mgr/Action.h"
#include "mgr/forward.h"

/// implement aggregated 'diskd' action
class DiskdAction: public Mgr::Action
{
protected:
    DiskdAction(const Mgr::CommandPointer &);

public:
    static Pointer Create(const Mgr::CommandPointer &);
    /* Action API */
    virtual const char *contentType(const String &) const override;
    virtual void add(const Mgr::Action &) override;
    virtual void pack(Ipc::TypedMsgHdr &) const override;
    virtual void unpack(const Ipc::TypedMsgHdr &) override;

protected:
    /* Action API */
    virtual void collect() override;
    virtual void dump(StoreEntry *) override;

private:
    mutable bool fmtYaml = false;
    DiskdStats data;
};

#endif /* SQUID_DISKD_ACTION_H */

