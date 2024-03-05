## Copyright (C) 1996-2025 The Squid Software Foundation and contributors
##
## Squid software is distributed under GPLv2+ license and includes
## contributions from numerous individuals and organizations.
## Please see the COPYING and CONTRIBUTORS files for details.
##

dnl check for --with-tdb option
AC_DEFUN_ONCE([SQUID_CHECK_LIBTDB],[
SQUID_AUTO_LIB(tdb,[Samba TrivialDB],[LIBTDB])
SQUID_CHECK_LIB_WORKS(tdb,[tdb],[
  AC_CHECK_HEADERS([sys/stat.h tdb.h],,,[
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
  ])
])
])
