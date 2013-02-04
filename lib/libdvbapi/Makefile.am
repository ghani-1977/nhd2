INCLUDES = \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/system \
	-I$(top_srcdir)/src/zapit/include \
	-I$(top_srcdir)/src/zapit/include/client \
	-I$(top_srcdir)/lib/connection \
	-I$(top_srcdir)/lib/libconfigfile \
	-I$(top_srcdir)/lib/libeventserver

if ENABLE_GSTREAMER
INCLUDES += @GST_CFLAGS@
else
if ENABLE_LIBEPLAYER3
INCLUDES += -I$(top_srcdir)/lib/libeplayer3/include
endif
endif

noinst_LIBRARIES = libdvbapi.a

libdvbapi_a_SOURCES = audio_cs.cpp dmx_cs.cpp playback_cs.cpp record_cs.cpp \
	video_cs.cpp

# dvb-ci
libdvbapi_a_SOURCES += dvb-ci.cpp \
	dvbci_session.cpp dvbci_mmi.cpp dvbci_camgr.cpp dvbci_appmgr.cpp dvbci_datetimemgr.cpp dvbci_resmgr.cpp



