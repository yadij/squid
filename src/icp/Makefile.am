## Copyright (C) 1996-2021 The Squid Software Foundation and contributors
##
## Squid software is distributed under GPLv2+ license and includes
## contributions from numerous individuals and organizations.
## Please see the COPYING and CONTRIBUTORS files for details.
##

include $(top_srcdir)/src/Common.am
include $(top_srcdir)/src/TestHeaders.am

noinst_LTLIBRARIES = libicp.la

libicp_la_SOURCES = \
	icp_opcode.cc \
	icp_opcode.h \
	icp_v2.cc \
	icp_v3.cc \
	ICP.h

icp_opcode.cc: icp_opcode.h $(srcdir)/../mk-string-arrays.awk
	$(AWK) -f $(srcdir)/../mk-string-arrays.awk ifile=icp/icp_opcode.h < $(srcdir)/icp_opcode.h > $@ || ($(RM) -f $@ && exit 1)

