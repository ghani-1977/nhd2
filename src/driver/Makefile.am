SUBDIRS = pictureviewer audiodec

INCLUDES = \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/zapit/include/zapit \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/libeventserver \
	-I$(top_srcdir)/lib/libnet \
	-I$(top_srcdir)/lib/libconfigfile \
	-I$(top_srcdir)/lib/connection \
	-I$(top_srcdir)/lib/libeventserver \
	-I$(top_srcdir)/lib/libxmltree \
	@FREETYPE_CFLAGS@

if BOXTYPE_COOLSTREAM
INCLUDES += -I$(top_srcdir)/src/zapit/include \
	-I$(top_srcdir)/lib/libcoolstream
else
INCLUDES += -I$(top_srcdir)/lib/libdvbapi
endif

if ENABLE_LCD
INCLUDES += -I$(top_srcdir)/lib/liblcddisplay
endif

if ENABLE_LIBEPLAYER3
INCLUDES += -I$(top_srcdir)/lib/libeplayer3/include
else
if ENABLE_GSTREAMER
INCLUDES += @GST_CFLAGS@
endif
endif

noinst_LIBRARIES = libneutrino_driver.a libneutrino_driver_netfile.a

libneutrino_driver_a_SOURCES = \
	ringbuffer.c \
	encoding.cpp \
	fontrenderer.cpp \
	framebuffer.cpp \
	fb_window.cpp \
	rcinput.cpp \
	vcrcontrol.cpp \
	audioplay.cpp \
	stream2file.cpp \
	screen_max.cpp \
	slotbuffer.c \
	file.cpp \
	audiometadata.cpp \
	audiofile.cpp \
	shutdown_count.cpp \
	genpsi.c \
	streamts.cpp \
	radiotext.cpp \
	radiotools.cpp \
	png.cpp \
	bmp.cpp \
	jpeg.cpp \
	gif.cpp \
	crw.cpp

if ENABLE_LCD
libneutrino_driver_a_SOURCES += lcdd.cpp	
else
libneutrino_driver_a_SOURCES += vfd.cpp
endif

if ENABLE_GRAPHLCD
libneutrino_driver_a_SOURCES += nglcd.cpp
endif

if USE_OPENGL
libneutrino_driver_a_SOURCES += \
	glthread.cpp
endif

libneutrino_driver_netfile_a_SOURCES = netfile.cpp



