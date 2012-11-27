INCLUDES = \
	-I$(top_srcdir)/src \
        -I$(top_srcdir)/src/zapit/include \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/libcoolstream \
	-I$(top_srcdir)/lib/libconfigfile \
	@FREETYPE_CFLAGS@

noinst_LIBRARIES = libtuxtxt.a

libtuxtxt_a_SOURCES = libtuxtxt.cpp tuxtxt.cpp

install-exec-local:
	install -d $(DESTDIR)$(CONFIGDIR)/tuxtxt
	install -m 0644 $(srcdir)/tuxtxt2.conf $(DESTDIR)$(CONFIGDIR)/tuxtxt
