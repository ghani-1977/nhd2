INCLUDES = \
	-I$(top_srcdir)/src/zapit/include \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/libcoolstream \
	@AVCODEC_CFLAGS@

AM_CPPFLAGS = -fno-rtti -fno-exceptions -D__STDC_CONSTANT_MACROS

noinst_LIBRARIES = libdvbsub.a

libdvbsub_a_SOURCES = dvbsub.cpp dvbsubtitle.cpp \
	tools.cpp PacketQueue.cpp helpers.cpp Debug.cpp
