## Copyright (C) 1996-2017 The Squid Software Foundation and contributors
##
## Squid software is distributed under GPLv2+ license and includes
## contributions from numerous individuals and organizations.
## Please see the COPYING and CONTRIBUTORS files for details.
##
#
# This file is supposed to run all the tests required to identify which
# configured modules are able to be built in this environment

# FIXME: de-duplicate $enable_auth_bearer list containing double entries.

#not specified. Inherit global
if test "x$enable_auth_bearer" = "x"; then
    enable_auth_bearer=$enable_auth
fi
#conflicts with global
if test "x$enable_auth_bearer" != "xno" -a "x$enable_auth" = "xno" ; then
    AC_MSG_ERROR([Bearer auth requested but auth disabled])
fi
#define list of modules to build
auto_auth_bearer_modules=no
if test "x$enable_auth_bearer" = "xyes" ; then
    SQUID_LOOK_FOR_MODULES([$srcdir/src/auth/bearer],[enable_auth_bearer])
  auto_auth_bearer_modules=yes
fi
#handle the "none" special case
if test "x$enable_auth_bearer" = "xnone" ; then
    enable_auth_bearer=""
fi

BEARER_AUTH_HELPERS=""
#enable_auth_bearer contains either "no" or the list of modules to be built
enable_auth_bearer="`echo $enable_auth_bearer| sed -e 's/,/ /g;s/  */ /g'`"
if test "x$enable_auth_bearer" != "xno" ; then
    AUTH_MODULES="$AUTH_MODULES bearer"
    AC_DEFINE([HAVE_AUTH_MODULE_BEARER],1,[Bearer auth module is built])
    for helper in $enable_auth_bearer; do
      dir="$srcdir/src/auth/bearer/$helper"

      # No helpers bundled yet.
      # See other auth helpers.m4 files for how to construct this list.

      # modules not yet converted to autoconf macros (or third party drop-in's)
      if test -f "$dir/config.test" && sh "$dir/config.test" "$squid_host_os"; then
        BUILD_HELPER="$helper"
      fi

      if test -d "$srcdir/src/auth/bearer/$helper"; then
        if test "$BUILD_HELPER" != "$helper"; then
          if test "x$auto_auth_basic_modules" = "xyes"; then
            AC_MSG_NOTICE([Bearer auth helper $helper ... found but cannot be built])
          else
            AC_MSG_ERROR([Bearer auth helper $helper ... found but cannot be built])
          fi
        else
          BEARER_AUTH_HELPERS="$BEARER_AUTH_HELPERS $BUILD_HELPER"
        fi
      else
        AC_MSG_ERROR([Bearer auth helper $helper ... not found])
      fi
    done
fi

AC_MSG_NOTICE([Bearer auth helpers to be built: $BEARER_AUTH_HELPERS])
AM_CONDITIONAL(ENABLE_AUTH_BEARER, test "x$enable_auth_bearer" != "xno")
AC_SUBST(BEARER_AUTH_HELPERS)
