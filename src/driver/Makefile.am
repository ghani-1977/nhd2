AM_CXXFLAGS = -fno-rtti -fno-exceptions

SUBDIRS = pictureviewer audiodec

INCLUDES = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/zapit/include \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/libeventserver \
	-I$(top_srcdir)/lib/libnet \
	-I$(top_srcdir)/lib/libconfigfile \
	-I$(top_srcdir)/lib/connection \
	-I$(top_srcdir)/lib/libeventserver \
	-I$(top_srcdir)/lib/libcoolstream \
	@FREETYPE_CFLAGS@

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
	vfd.cpp \
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
	radiotools.cpp

libneutrino_driver_a_SOURCES += \
	png.cpp \
	bmp.cpp \
	jpeg.cpp \
	gif.cpp \
	crw.cpp

libneutrino_driver_netfile_a_SOURCES = netfile.cpp

if ENABLE_GRAPHLCD
libneutrino_driver_a_SOURCES += nglcd.cpp
endif

