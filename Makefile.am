DISTCHECK_CONFIGURE_FLAGS = --enable-introspection --enable-gtk-doc

-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --add-include-path=$(srcdir)
INTROSPECTION_COMPILER_ARGS = --includedir=$(srcdir)

SUBDIRS=po dbind xml atspi bus registryd doc

ACLOCAL_AMFLAGS=-I m4 ${ACLOCAL_FLAGS}

EXTRA_DIST = \
	atspi-2-uninstalled.pc.in \
	atspi-2.pc.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = atspi-2.pc

DISTCLEANFILES = \
	atspi-2-uninstalled.pc \
	atspi-2.pc
