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
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/libeventserver \
	-I$(top_srcdir)/lib/libnet \
	-I$(top_srcdir)/lib/libconfigfile \
	-I$(top_srcdir)/lib/libxmltree \
	-I$(top_srcdir)/lib/libcoolstream \
	@CURL_CFLAGS@ \
	@FREETYPE_CFLAGS@

if ENABLE_LIBEPLAYER3
INCLUDES += -I$(top_srcdir)/lib/libeplayer3/include
endif

if ENABLE_UPNP
INCLUDES += -I$(top_srcdir)/lib/libupnpclient
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
	scan_setup.cpp zapit_setup.cpp proxyserver_setup.cpp themes.cpp

libneutrino_gui2_a_SOURCES = \
	color.cpp motorcontrol.cpp hdd_menu.cpp \
	alphasetup.cpp dvbsub_select.cpp psisetup.cpp opkg_manager.cpp

if ENABLE_UPNP
libneutrino_gui2_a_SOURCES += upnpbrowser.cpp
endif

if !BOXTYPE_COOLSTREAM
libneutrino_gui2_a_SOURCES += cam_menu.cpp
endif

libtimerlist_a_SOURCES = timerlist.cpp

if ENABLE_GRAPHLCD
libneutrino_gui_a_SOURCES += \
	glcdsetup.cpp
endif

