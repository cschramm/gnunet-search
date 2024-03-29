# This file is part of GNUnet.
# (C) 2001-2011 Christian Grothoff (and other contributing authors)
#
# GNUnet is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published
# by the Free Software Foundation; either version 2, or (at your
# option) any later version.
#
# GNUnet is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNUnet; see the file COPYING.  If not, write to the
# Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.
#
#
# Process this file with autoconf to produce a configure script.
#
AC_PREREQ(2.61)
AC_INIT([gnunet-search],[0.0.0],[bug-gnunet@gnu.org])
AM_INIT_AUTOMAKE([gnunet-search], [0.0.0])
AM_CONFIG_HEADER(gnunet_search_config.h)

AH_TOP([#define _GNU_SOURCE  1])

AC_ISC_POSIX
AC_PROG_AWK
AC_PROG_CC

AC_PROG_MKDIR_P
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_LIBTOOL_WIN32_DLL
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_CANONICAL_HOST

# dynamic libraries/plugins
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

AC_SYS_LARGEFILE
AC_FUNC_FSEEKO

CFLAGS="-std=c99 -Wall $CFLAGS"
# use '-fno-strict-aliasing', but only if the compiler can take it
if gcc -fno-strict-aliasing -S -o /dev/null -xc /dev/null >/dev/null 2>&1;
then
 CFLAGS="-fno-strict-aliasing $CFLAGS"
fi


# Check system type
case "$host_os" in
*darwin* | *rhapsody* | *macosx*)
     AC_DEFINE_UNQUOTED(OSX,1,[This is an OS X system])
     CFLAGS="-no-cpp-precomp $CFLAGS"
     LDFLAGS="-flat_namespace -undefined suppress $LDFLAGS"
     ;;
linux*)
     AC_DEFINE_UNQUOTED(LINUX,1,[This is a Linux system])
     ;;
freebsd*)
     AC_DEFINE_UNQUOTED(SOMEBSD,1,[This is a BSD system])
     AC_DEFINE_UNQUOTED(FREEBSD,1,[This is a FreeBSD system])
     ;;
openbsd*)
     AC_DEFINE_UNQUOTED(SOMEBSD,1,[This is a BSD system])
     AC_DEFINE_UNQUOTED(OPENBSD,1,[This is an OpenBSD system])
     ;;
netbsd*)
     AC_DEFINE_UNQUOTED(SOMEBSD,1,[This is a BSD system])
     AC_DEFINE_UNQUOTED(NETBSD,1,[This is a NetBSD system])
     ;;
*solaris*)
     AC_DEFINE_UNQUOTED(SOLARIS,1,[This is a Solaris system])
     AC_DEFINE_UNQUOTED(_REENTRANT,1,[Need with solaris or errno doesnt work])
     build_target="solaris"
     ;;
*arm-linux*)
     AC_DEFINE_UNQUOTED(LINUX,1,[This is a Linux system])
     ;;
*cygwin*)
     AC_DEFINE_UNQUOTED(CYGWIN,1,[This is a Cygwin system])
     AC_DEFINE_UNQUOTED(WINDOWS,1,[This is a Windows system])
     AC_CHECK_LIB(intl, gettext)
     LDFLAGS="$LDFLAGS -no-undefined"
     build_target="cygwin"
     ;;
*mingw*)
     AC_DEFINE_UNQUOTED(MINGW,1,[This is a MinGW system])
     AC_DEFINE_UNQUOTED(WINDOWS,1,[This is a Windows system])
     AC_DEFINE_UNQUOTED(_WIN32,1,[This is a Windows system])
     AC_CHECK_LIB(intl, gettext)
     LDFLAGS="$LDFLAGS -no-undefined -Wl,--export-all-symbols -lws2_32"
     CFLAGS="-mms-bitfields $CFLAGS"
     build_target="mingw"
     ;;
*)
     AC_MSG_RESULT(Unrecognised OS $host_os)
     AC_DEFINE_UNQUOTED(OTHEROS,1,[Some strange OS])
;;
esac

AM_CONDITIONAL(MINGW,   test "$build_target" = "mingw")

# check for gettext
AM_GNU_GETTEXT_VERSION([0.18.1])
AM_GNU_GETTEXT([external])

AC_CHECK_HEADERS([errno.h stdio.h unistd.h locale.h sys/stat.h sys/types.h langinfo.h libintl.h unistd.h stddef.h argz.h sys/socket.h netinet/in.h stdarg.h])

# test for GNUnet core
gnunet=0
lookin=${prefix}
backup_LIBS="$LIBS"	
backup_CFLAGS="$CFLAGS	"
backup_CPPFLAGS="$CPPFLAGS"
GNUNET_CFLAGS=""
GNUNET_CPPFLAGS=""
GNUNET_LIBS=""
AC_MSG_CHECKING(for GNUnet core)
AC_ARG_WITH(gnunet,
   [  --with-gnunet=PFX       Base of GNUnet installation],
   [AC_MSG_RESULT([$with_gnunet])
    case $with_gnunet in
      no)
        lookin=""
        ;;
      yes)
        lookin="${prefix}"
        ;;
      *)
        lookin="$with_gnunet"
        ;;
    esac
   ],
   [
     AC_MSG_RESULT([--with-gnunet not specified])
     PKG_CHECK_MODULES([GNUNET], [gnunetutil >= 0.9.0], gnunet=1)
   ]
)

if test "x$gnunet" == "x0" -a ! "x$lookin" == "x"
then
  AC_MSG_CHECKING(for GNUnet util library in $lookin)
  GNUNET_LIBS="-L${lookin}/lib"
  GNUNET_CFLAGS="-I${lookin}/include"
  GNUNET_CPPFLAGS="-I${lookin}/include"
  LIBS="$GNUNET_LIBS $backup_LIBS"
  CFLAGS="$GNUNET_CFLAGS $backup_CFLAGS"
  CPPFLAGS="$GNUNET_CPPFLAGS $backup_CPPFLAGS"
  AC_CHECK_HEADERS([gnunet/gnunet_util_lib.h],
    AC_CHECK_LIB([gnunetutil], [GNUNET_xfree_],
      [
        gnunet=1
        EXT_LIB_PATH="-L${lookin}/lib $EXT_LIB_PATH"
      ]
    ),,[#include <gnunet/platform.h>]
  )
fi

if test "x$gnunet" == "x0"
then
  AC_MSG_ERROR([gnunet-gtk requires GNUnet])
fi

AC_SUBST(GNUNET_CFLAGS)
AC_SUBST(GNUNET_LIBS)
LIBS="$backup_LIBS"
CFLAGS="$backup_CFLAGS"
CPPFLAGS="$backup_CPPFLAGS"

clearsilver=1
zlib=0
microhttpd=1
jansson=1
for d in "/usr/include" "/usr/local/include" "{$prefix}/include"; do
	if test -d "${d}/ClearSilver"; then
		CS_INCLUDES="-I${d}/ClearSilver"
	fi
done
CPPFLAGS="${CPPFLAGS} ${CS_INCLUDES}"
AC_CHECK_HEADERS([ClearSilver.h],, [clearsilver=0])
AC_CHECK_HEADERS([microhttpd.h],, [microhttpd=0])
AC_CHECK_HEADERS([jansson.h],, [jansson=0])
AC_HAVE_LIBRARY([neo_cs],, [clearsilver=0])
AC_HAVE_LIBRARY([neo_utl],, [clearsilver=0])
AC_HAVE_LIBRARY([neo_cgi],, [clearsilver=0])
AC_HAVE_LIBRARY([z], [zlib=1])
AC_HAVE_LIBRARY([microhttpd],, [microhttpd=0])
AC_HAVE_LIBRARY([jansson],, [jansson=0])

if test "x$clearsilver" == "x0"
then
  AC_MSG_ERROR([gnunet-search requires ClearSilver])
fi

if test "x$microhttpd" == "x0"
then
  AC_MSG_ERROR([gnunet-search requires libmicrohttpd])
fi

if test "x$zlib" == "x0"
then
  AC_MSG_ERROR([ClearSilver requires zlib])
fi

if test "x$jansson" == "x0"
then
  AC_MSG_ERROR([gnunet-search requires jansson])
fi

AC_DEFINE_DIR([PACKAGE_DATA], [datarootdir], [The directory for installing read-only architecture-independent data])

# Set PACKAGE_SOURCE_DIR in gnunet_gtk_config.h.
packagesrcdir=`cd $srcdir && pwd`
AC_DEFINE_UNQUOTED(PACKAGE_SOURCE_DIR, "${packagesrcdir}", [source dir])

AC_OUTPUT([ po/Makefile.in
Makefile
src/Makefile
src/include/Makefile
src/search/Makefile
])
