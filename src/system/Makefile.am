INCLUDES = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/zapit/include \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/connection \
	-I$(top_srcdir)/lib/libeventserver \
	-I$(top_srcdir)/lib/libnet \
	-I$(top_srcdir)/lib/libconfigfile \
	-I$(top_srcdir)/lib/libmd5sum \
	-I$(top_srcdir)/lib/libxmltree \
	-I$(top_srcdir)/lib/libcoolstream \
	@FREETYPE_CFLAGS@

if !ENABLE_GSTREAMER
INCLUDES += -I$(top_srcdir)/lib/libeplayer3/include
endif

noinst_LIBRARIES = libneutrino_system.a

libneutrino_system_a_SOURCES = \
	safe_system.c \
	localize.cpp setting_helpers.cpp debug.cpp \
	ping.c flashtool.cpp httptool.cpp \
	settings.cpp lastchannel.cpp \
	configure_network.cpp fsmounter.cpp
