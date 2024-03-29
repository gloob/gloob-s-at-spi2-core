AC_INIT([at-spi2-core], [2.1.92], [accessibility-atspi@lists.linux-foundation.org])
AC_PREREQ([2.59])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR([m4])

AT_SPI_CORE_MAJOR_VERSION=0
AT_SPI_CORE_MINOR_VERSION=1
AT_SPI_CORE_MICRO_VERSION=3
AT_SPI_CORE_VERSION="$AT_SPI_CORE_MAJOR_VERSION.$AT_SPI_CORE_MINOR_VERSION"
AC_SUBST(AT_SPI_CORE_VERSION)

LT_RELEASE=$AT_SPI_CORE_VERSION
LT_CURRENT=0
LT_REVISION=1
LT_AGE=0
LT_VERSION_INFO='-version-info ${LT_CURRENT}:${LT_REVISION}:${LT_AGE}'
AC_SUBST(LT_VERSION_INFO)
AC_SUBST(LT_RELEASE)
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)

AM_INIT_AUTOMAKE([1.9 -Wall foreign no-dist-gzip dist-xz])

AC_PROG_CC
LT_INIT([disable-static])
PKG_PROG_PKG_CONFIG

AC_CONFIG_HEADERS([config.h])

dnl translation of at-spi strings
IT_PROG_INTLTOOL([0.40.0])


GETTEXT_PACKAGE="${PACKAGE}"
AC_SUBST(GETTEXT_PACKAGE)

PKG_CHECK_MODULES(DBUS, [dbus-1 >= 1.0])
AC_SUBST(DBUS_LIBS)
AC_SUBST(DBUS_CFLAGS)

PKG_CHECK_MODULES(GLIB, [glib-2.0 >= 2.26.0])
AC_SUBST(GLIB_LIBS)
AC_SUBST(GLIB_CFLAGS)
AC_CHECK_LIB([glib-2.0], [g_ptr_array_new_with_free_func], [:],
	[AC_MSG_ERROR([The version of glib-2.0 found does not define ]
		[g_ptr_array_new_with_free_func. Please upgrade glib.])],
	[$GLIB_LIBS])

PKG_CHECK_MODULES(GOBJ, [gobject-2.0 >= 2.0.0])
AC_SUBST(GOBJ_LIBS)
AC_SUBST(GOBJ_CFLAGS)

PKG_CHECK_MODULES(GIO, [gio-2.0 >= 2.28])
AC_SUBST(GIO_LIBS)
AC_SUBST(GIO_CFLAGS)

# --------------------------------------------------------------------
# Find DL functionality

AC_CHECK_LIB(c, dlopen, DL_LIBS="",
        AC_CHECK_LIB(dl, dlopen, DL_LIBS="-ldl",
                AC_MSG_ERROR([Could not find a library with the dlopen function])
        )
)

AC_SUBST(DL_LIBS)

AC_PATH_XTRA

if test x$no_x = xyes ; then
	AC_MSG_ERROR([X development libraries not found])
else
	X_LIBS="$X_LIBS $X_PRE_LIBS -lX11 $X_EXTRA_LIBS"
fi
AC_SUBST(X_LIBS)

save_LIBS="$LIBS"
LIBS=""
CPPFLAGS="$CPPFLAGS $X_CFLAGS"
AC_CHECK_LIB(Xtst, XTestFakeKeyEvent, XTST_LIBS=-lXtst,[
	AC_MSG_ERROR([Couldn't find the Xtst library. Check config.log])],
	$X_LIBS)
AC_SUBST(XTST_LIBS)
LIBS="$LIBS $save_LIBS"

save_LIBS="$LIBS"
LIBS=""
AC_CHECK_LIB(X11, XkbQueryExtension, have_xkb="maybe", have_xkb="no", $X_LIBS)
if test "x$have_xkb" = "xmaybe"; then 
	AC_CHECK_HEADER([X11/XKBlib.h], have_xkb=yes)
fi
if test "x$have_xkb" = "xyes"; then
	AC_DEFINE([HAVE_XKB], [], [Xkb is present])
fi
LIBS="$LIBS $save_LIBS"

save_LIBS="$LIBS"
LIBS=""
AC_CHECK_LIB(Xi, XOpenDevice, have_xinput="maybe", have_xinput="no", $X_LIBS)
if test "x$have_xinput" = "xmaybe"; then
	save_LIBS="$LIBS"
	LIBS="$LIBS $X_LIBS"
	AC_CHECK_HEADER([X11/extensions/XInput.h], [have_xinput="yes"])
	LIBS="$save_LIBS"
fi
if test "x$have_xinput" != "xyes"; then
	AC_MSG_ERROR([Couldn't find the XInput library. Check config.log for details])
fi
XINPUT_LIBS="-lXi"
AC_SUBST(XINPUT_LIBS)
LIBS="$LIBS $save_LIBS"

save_LIBS="$LIBS"
LIBS=""
AC_ARG_ENABLE(xevie,
	[AS_HELP_STRING([--enable-xevie],
		[Build with XEViE support [default=yes]])],
	enable_xevie="$enableval", enable_xevie=yes)

if test x$enable_xevie = xyes ; then
	have_xevie=
	AC_CHECK_LIB(Xext, XevieStart, have_xevie="yes", have_xevie="no", $X_LIBS)

	if test "x$have_xevie" = "xyes"; then
		XEVIE_LIBS="-lXext"
		AC_DEFINE([HAVE_XEVIE], [], [Xevie is present])
	else
	   AC_CHECK_LIB(Xevie, XevieQueryVersion, have_xevie="maybe",,$X_LIBS)
	   if test "x$have_xevie" = "xmaybe"; then
	      AC_CHECK_HEADER(X11/extensions/Xevie.h, have_xevie=yes, [],
	      [[#include <X11/Xlib.h>
]])
	      if test "x$have_xevie" = "xyes"; then
	      	      XEVIE_LIBS="-lXext -lXevie"	       
		      AC_DEFINE([HAVE_XEVIE], [], [Xevie is present])
	      fi
	   fi	
	fi
	AC_SUBST(XEVIE_LIBS)
fi
LIBS="$LIBS $save_LIBS"

AC_ARG_WITH([dbus_daemondir],
	[AS_HELP_STRING([--with-dbus-daemondir=<directory>],
		[Directory where the DBUS daemon is installed])],
    [DBUS_DAEMON="$with_dbus_daemondir/dbus-daemon"],
    [AC_PATH_PROG([DBUS_DAEMON], [dbus-daemon], [XXXDBUSDAEMONNOTFOUND],
		[$PATH$PATH_SEPARATOR/sbin$PATH_SEPARATOR/usr/sbin$PATH_SEPARATOR]dnl
[/libexec$PATH_SEPARATOR/usr/libexec$PATH_SEPARATOR/usr/local/bin]dnl
[$PATH_SEPARATOR/usr/pkg/bin])]
	)
if test "$DBUS_DAEMON" = "XXXDBUSDAEMONNOTFOUND"; then
	DBUS_DAEMON = "$bindir/dbus-daemon"
	AC_MSG_WARN([at-spi2 relies on dbus, and the dbus-daemon was not found])
	AC_MSG_WARN([we assume that it will be installed in "$bindir"])
fi
AC_SUBST(DBUS_DAEMON)

AC_ARG_WITH(dbus-services,
           [AS_HELP_STRING([--with-dbus-services=<directory>],
           [where D-BUS services directory is])])
if ! test -z "$with_dbus_services" ; then
           DBUS_SERVICES_DIR="$with_dbus_services"
else
           DBUS_SERVICES_DIR="$datadir/dbus-1/services"
fi
AC_SUBST(DBUS_SERVICES_DIR)

GOBJECT_INTROSPECTION_CHECK([0.9.6])

AC_SUBST(LIBTOOL_EXPORT_OPTIONS)

dnl find sizes & alignments
orig_CPPFLAGS=$CPPFLAGS
CPPFLAGS="$CPPFLAGS $DBUS_CFLAGS"
DBIND_CHECK_ALIGNOF(char)
DBIND_CHECK_ALIGNOF(dbus_bool_t)
DBIND_CHECK_ALIGNOF(dbus_int16_t)
DBIND_CHECK_ALIGNOF(dbus_int32_t)
DBIND_CHECK_ALIGNOF(dbus_int64_t)
DBIND_CHECK_ALIGNOF(double)
DBIND_CHECK_ALIGNOF(dbind_pointer)
DBIND_CHECK_ALIGNOF(dbind_struct)

GTK_DOC_CHECK([1.09])

if test "x$GCC" = xyes; then
	CFLAGS="$CFLAGS -Werror-implicit-function-declaration"
fi
CPPFLAGS=$orig_CPPFLAGS 
AC_CONFIG_FILES([Makefile po/Makefile.in
	xml/Makefile
	dbind/Makefile
dbind/dbind-config.h
	atspi/Makefile
	registryd/Makefile
	bus/Makefile
doc/Makefile
doc/libatspi/Makefile
atspi-2.pc
atspi-2-uninstalled.pc
])

AC_OUTPUT
