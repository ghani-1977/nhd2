INCLUDES = \
	-I$(top_srcdir)/src \
        -I$(top_srcdir)/lib \
	-I$(top_srcdir)/src/zapit/include \
	-I$(top_srcdir)/lib/libconfigfile \
	-I$(top_srcdir)/lib/libcoolstream \
        @FREETYPE_CFLAGS@ \
        @VORBISIDEC_CFLAGS@

if !ENABLE_GSTREAMER
INCLUDES += -I$(top_srcdir)/lib/libeplayer3/include
endif

noinst_LIBRARIES = libneutrino_driver_audiodec.a

libneutrino_driver_audiodec_a_SOURCES = \
	basedec.cpp \
	cdrdec.cpp \
	mp3dec.cpp \
	wavdec.cpp \
	tag.c \
	crc.c \
	vis.cpp \
	flacdec.cpp

