## Copyright (C) 1996-2018 The Squid Software Foundation and contributors
##
## Squid software is distributed under GPLv2+ license and includes
## contributions from numerous individuals and organizations.
## Please see the COPYING and CONTRIBUTORS files for details.
##

include $(top_srcdir)/src/Common.am
include $(top_srcdir)/src/TestHeaders.am

SUBDIRS= id_rewriters

noinst_LTLIBRARIES = libstore.la libstoremeta.la

libstore_la_SOURCES= \
	Controlled.h \
	Controller.cc \
	Controller.h \
	Disk.cc \
	Disk.h \
	Disks.cc \
	Disks.h \
	forward.h \
	LocalSearch.cc \
	LocalSearch.h \
	Storage.h

libstore_la_LIBADD= libstoremeta.la

# separate library shared with helper binaries
libstoremeta_la_SOURCES= \
	MetaMd5.cc \
	MetaMd5.h \
	MetaObjSize.h \
	MetaStd.cc \
	MetaStd.h \
	MetaStdLfs.cc \
	MetaStdLfs.h \
	MetaTlv.cc \
	MetaTlv.h \
	MetaUrl.cc \
	MetaUrl.h \
	MetaVary.cc \
	MetaVary.h
