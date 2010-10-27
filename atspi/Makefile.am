lib_LTLIBRARIES = libatspi.la

libatspi_la_LDFLAGS = @LDFLAGS@ @LT_VERSION_INFO@ @LIBTOOL_EXPORT_OPTIONS@ -no-undefined

libatspi_la_CFLAGS = $(DBUS_GLIB_CFLAGS) \
		    $(DBIND_CFLAGS)     \
                    -I$(top_srcdir)

libatspi_LDADD = $(DBUS_GLIB_LIBS) \
		    $(top_builddir)/dbind/libdbind.la

libatspidir = $(includedir)/at-spi-1.0/atspi

libatspi_la_SOURCES =		\
	atspi.h			\
	atspi-accessible.c \
	atspi-accessible.h \
	atspi-application.c \
	atspi-application.h \
	atspi-constants.h \
	atspi-event-types.h \
	atspi-listener.c \
	atspi-listener.h \
	atspi-listener-private.h \
	atspi-misc.c \
	atspi-misc.h \
	atspi-misc-private.h \
	atspi-private.h \
	atspi-registry.c \
	atspi-registry.h \
	atspi-stateset.c \
	atspi-stateset.h

libatspi_la_LIBADD =		        \
	$(LIBATSPI_LIBS)                 \
	$(DBIND_LIBS)			\
	$(X_LIBS) \
	$(DBUS_GLIB_LIBS)

#BUILT_SOURCES = atspi-constants.h

#CLEANFILES = atspi-constants.h

#atspi-constants.h: $(top_srcdir)/xml/spec.xml $(top_srcdir)/tools/c-constants-gen.py
#	python $(top_srcdir)/tools/c-constants-gen.py Atspi $(top_srcdir)/xml/spec.xml atspi-constants

if HAVE_INTROSPECTION
INTROSPECTION_FILES = $(libatspi_la_SOURCES)

Atspi-2.0.gir: $(INTROSPECTION_SCANNER) \
		libatspi.la \
		$(INTROSPECTION_FILES)
	$(AM_V_GEN)$(INTROSPECTION_SCANNER) -v \
		--namespace Atspi \
		--nsversion=2.0 \
		-I$(top_builddir) \
		-I$(top_srcdir) \
		--include=GLib-2.0 \
		--include=GObject-2.0 \
		--libtool="$(top_builddir)/libtool" \
		--library=libatspi.la \
		--library=$(top_builddir)/dbind/libdbind.la \
		--pkg dbus-glib-1 \
		--warn-all \
		--output $@ \
		$(INTROSPECTION_FILES)

girdir = $(datadir)/gir-1.0
gir_DATA = Atspi-2.0.gir

%.typelib: %.gir $(INTROSPECTION_COMPILER)
	$(AM_V_GEN)$(top_builddir)/libtool --mode=execute \
	    $(INTROSPECTION_COMPILER) \
	        --includedir=$(srcdir) \
	        --includedir=. \
	        $(INTROSPECTION_COMPILER_OPTS) $< -o $(@F)

typelibsdir = $(libdir)/girepository-1.0/
typelibs_DATA = $(gir_DATA:.gir=.typelib)

CLEANFILES = \
    $(gir_DATA) \
    $(typelibs_DATA)

endif