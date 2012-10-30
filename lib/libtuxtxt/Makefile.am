INCLUDES = \
	-I$(top_srcdir)/src \
        -I$(top_srcdir)/src/zapit/include \
	-I$(top_srcdir)/lib/libcoolstream \
	-I$(top_srcdir)/lib/libconfigfile

AM_CPPFLAGS = $(FREETYPE_CFLAGS) -fno-rtti -fno-exceptions

noinst_LIBRARIES = libtuxtxt.a

libtuxtxt_a_SOURCES = libtuxtxt.cpp tuxtxt.cpp

install-exec-local:
	install -d $(DESTDIR)$(CONFIGDIR)/tuxtxt
	install -m 0644 $(srcdir)/tuxtxt2.conf $(DESTDIR)$(CONFIGDIR)/tuxtxt
