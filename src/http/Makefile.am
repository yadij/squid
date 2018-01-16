## Copyright (C) 1996-2023 The Squid Software Foundation and contributors
##
## Squid software is distributed under GPLv2+ license and includes
## contributions from numerous individuals and organizations.
## Please see the COPYING and CONTRIBUTORS files for details.
##

include $(top_srcdir)/src/Common.am

SUBDIRS = \
	one \
	two \
	url_rewriters

noinst_LTLIBRARIES = libhttp.la

libhttp_la_SOURCES = \
	ContentLengthInterpreter.cc \
	ContentLengthInterpreter.h \
	FrameType.cc \
	FrameType.h \
	Message.cc \
	Message.h \
	MethodType.cc \
	MethodType.h \
	Parser.cc \
	Parser.h \
	ProtocolVersion.h \
	RegisteredHeaders.cc \
	RegisteredHeaders.h \
	RegisteredHeadersHash.cci \
	RequestMethod.cc \
	RequestMethod.h \
	StateFlags.h \
	StatusCode.cc \
	StatusCode.h \
	StatusLine.cc \
	StatusLine.h \
	Stream.cc \
	Stream.h \
	forward.h

libhttp_la_LIBADD= \
	one/libhttp1.la \
	two/libhttp2.la

FrameType.cc: FrameType.h $(top_srcdir)/src/mk-string-arrays.awk
	($(AWK) -f $(top_srcdir)/src/mk-string-arrays.awk sbuf=1 < $(srcdir)/FrameType.h >$@) \
		|| ($(RM) -f $@ && exit 1)

MethodType.cc: MethodType.h $(top_srcdir)/src/mk-string-arrays.awk
	($(AWK) -f $(top_srcdir)/src/mk-string-arrays.awk sbuf=1 < $(srcdir)/MethodType.h | \
		sed -e 's%METHOD_%%' -e 's%_C%-C%' >$@) || ($(RM) -f $@ && exit 1)

CLEANFILES += FrameType.cc MethodType.cc

EXTRA_DIST = RegisteredHeadersHash.gperf
