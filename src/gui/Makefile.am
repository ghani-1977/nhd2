AM_CPPFLAGS = -fno-rtti -fno-exceptions

BUILT_SOURCES  = svn_version.h

svn_version.h:
	@if test -d .svn ; then \
		sleep 180 && rm svn_version.h & echo "#define SVNVERSION \"$$(svnversion -n || echo svn_oops!)\" " > svn_version.h ; \
	else \
		rm svn_version.h; echo '#define BUILT_DATE "'`date`'"' > svn_version.h ; \
	fi

noinst_HEADERS = svn_version.h

SUBDIRS = widget bedit

INCLUDES = \
	-I$(top_srcdir)/daemons \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/zapit/include \
	-I$(top_srcdir)/lib/libeventserver \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/libnet \
	-I$(top_srcdir)/lib/libconfigfile \
	-I$(top_srcdir)/lib/libxmltree \
	-I$(top_srcdir)/lib/libupnpclient \
	-I$(top_srcdir)/lib/libcoolstream \
	@CURL_CFLAGS@ \
	@FREETYPE_CFLAGS@

if !ENABLE_GSTREAMER
INCLUDES += -I$(top_srcdir)/lib/libeplayer3/include
else
INCLUDES += @GST_CFLAGS@
endif

noinst_LIBRARIES = libtimerlist.a libneutrino_gui.a libneutrino_gui2.a

libneutrino_gui_a_SOURCES = \
	bouquetlist.cpp channellist.cpp eventlist.cpp pluginlist.cpp \
	favorites.cpp sleeptimer.cpp screensetup.cpp \
	epgview.cpp infoviewer.cpp scan.cpp update.cpp \
	filebrowser.cpp audioplayer.cpp nfs.cpp pictureviewer.cpp \
	movieplayer.cpp rc_lock.cpp \
	timeosd.cpp epgplus.cpp epg_menu.cpp \
	streaminfo2.cpp dboxinfo.cpp \
	plugins.cpp imageinfo.cpp audio_select.cpp moviebrowser.cpp movieinfo.cpp \
	scan_setup.cpp zapit_setup.cpp proxyserver_setup.cpp

libneutrino_gui2_a_SOURCES = \
	color.cpp motorcontrol.cpp hdd_menu.cpp \
	alphasetup.cpp dvbsub_select.cpp upnpbrowser.cpp

libneutrino_gui2_a_SOURCES += \
	alphasetup.cpp psisetup.cpp

if BOXTYPE_CUBEREVO
libneutrino_gui2_a_SOURCES += cam_menu.cpp
endif

if BOXTYPE_CUBEREVO_MINI
libneutrino_gui2_a_SOURCES += cam_menu.cpp
endif

if BOXTYPE_CUBEREVO_MINI2
libneutrino_gui2_a_SOURCES += cam_menu.cpp
endif

if BOXTYPE_CUBEREVO_9500HD
libneutrino_gui2_a_SOURCES += cam_menu.cpp
endif

if BOXTYPE_DUCKBOX
libneutrino_gui2_a_SOURCES += cam_menu.cpp
endif

if BOXTYPE_GIGABLUE
libneutrino_gui2_a_SOURCES += cam_menu.cpp
endif

if BOXTYPE_DREAMBOX
libneutrino_gui2_a_SOURCES += cam_menu.cpp
endif

if BOXTYPE_XTREND
libneutrino_gui2_a_SOURCES += cam_menu.cpp
endif

libtimerlist_a_SOURCES = timerlist.cpp

if ENABLE_GRAPHLCD
libneutrino_gui_a_SOURCES += \
	glcdsetup.cpp
endif

