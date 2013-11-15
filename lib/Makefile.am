SUBDIRS = \
	connection \
	libeventserver \
	sectionsdclient \
	timerdclient \
	libconfigfile \
	libmd5sum \
	libnet \
	libxmltree \
	libtuxtxt \
	libdvbsub  \
	jsoncpp

if !BOXTYPE_COOLSTREAM
SUBDIRS += libdvbapi
endif

if ENABLE_LIBEPLAYER3
SUBDIRS += libeplayer3
endif

if ENABLE_LCD
SUBDIRS += liblcddisplay
endif

if ENABLE_UPNP
SUBDIRS += libupnpclient
endif




