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
	libdvbsub \
	libcoolstream

if !ENABLE_GSTREAMER
SUBDIRS += libeplayer3
endif

if ENABLE_LCD
SUBDIRS += liblcddisplay
endif

if ENABLE_UPNP
SUBDIRS += libupnpclient
endif




