/*
 * Copyright (C) 1996-2017 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef _SQUID_SRC_DISKIO_DISKDAEMON_DISKDSTATS_H
#define _SQUID_SRC_DISKIO_DISKDAEMON_DISKDSTATS_H

/* DEBUG: section 79    Squid-side DISKD I/O functions. */

class DiskdStats
{
public:
    DiskdStats operator +=(const DiskdStats &stats) {
        sent_count += stats.sent_count;
        recv_count += stats.recv_count;
        if (stats.max_away > max_away)
            max_away = stats.max_away;
        if (stats.max_shmuse > max_shmuse)
            max_shmuse += stats.max_shmuse;
        open_fail_queue_len += stats.open_fail_queue_len;
        block_queue_len += stats.block_queue_len;

        open += stats.open;
        create += stats.create;
        close += stats.close;
        unlink += stats.unlink;
        read += stats.read;
        write += stats.write;
        return *this;
    }

    int open_fail_queue_len = 0;
    int block_queue_len = 0;
    int max_away = 0;
    int max_shmuse = 0;
    int shmbuf_count = 0;
    int sent_count = 0;
    int recv_count = 0;
    int sio_id = 0;

    class iops {
    public:
        iops &operator +=(const iops &rhs) {
            ops += rhs.ops;
            success += rhs.success;
            fail += rhs.fail;
            return *this;
        }
        int ops = 0;
        int success = 0;
        int fail = 0;
    }
    open, create, close, unlink, read, write;
};

extern DiskdStats diskd_stats;

#endif /* _SQUID_SRC_DISKIO_DISKDAEMON_DISKDSTATS_H */
