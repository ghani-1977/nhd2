AM_CPPFLAGS = -fno-rtti -fno-exceptions

SUBDIRS = zapit gui daemonc driver system timerd nhttpd sectionsd

INCLUDES = \
	-I$(top_srcdir)/daemons \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/src/zapit/include \
	-I$(top_srcdir)/lib/connection \
	-I$(top_srcdir)/lib/libeventserver \
	-I$(top_srcdir)/lib/libconfigfile \
	-I$(top_srcdir)/lib/libnet \
	-I$(top_srcdir)/lib/libxmltree \
	-I$(top_srcdir)/lib/libmd5sum \
	-I$(top_srcdir)/lib/libupnpclient \
	-I$(top_srcdir)/lib/libcoolstream \
	@FREETYPE_CFLAGS@

if !ENABLE_GSTREAMER
INCLUDES += -I$(top_srcdir)/lib/libeplayer3
endif

bin_PROGRAMS = neutrino

neutrino_SOURCES = neutrino_menue.cpp neutrino.cpp

neutrino_LDADD = \
	daemonc/libneutrino_daemonc.a \
	gui/bedit/libneutrino_gui_bedit.a \
	gui/libtimerlist.a \
	gui/libneutrino_gui.a \
	gui/widget/libneutrino_gui_widget.a \
	gui/widget/libneutrino_gui_widget2.a \
	driver/pictureviewer/libneutrino_pictureviewer.a \
	system/libneutrino_system.a \
	driver/libneutrino_driver.a \
	gui/movieinfo.o \
	driver/audiodec/libneutrino_driver_audiodec.a \
	driver/libneutrino_driver_netfile.a \
	gui/libneutrino_gui2.a \
	sectionsd/libsectionsd.a \
	timerd/libtimerd.a \
	zapit/src/libzapit.a \
	nhttpd/libnhttpd.a \
	nhttpd/tuxboxapi/libnhttpd_tuxboxapi.a \
	nhttpd/yhttpd_mods/libyhttpdmods.a \
	nhttpd/yhttpd_core/libyhttpd.a \
	$(top_builddir)/lib/sectionsdclient/libsectionsdclient.a \
	$(top_builddir)/lib/timerdclient/libtimerdclient.a \
	$(top_builddir)/src/zapit/lib/libzapitclient.a \
	$(top_builddir)/lib/libconfigfile/libtuxbox-configfile.a \
	$(top_builddir)/lib/connection/libtuxbox-connection.a \
	$(top_builddir)/lib/libeventserver/libtuxbox-eventserver.a \
	$(top_builddir)/lib/libxmltree/libtuxbox-xmltree.a \
	$(top_builddir)/lib/libnet/libtuxbox-net.a \
	$(top_builddir)/lib/libmd5sum/libtuxbox-md5sum.a \
	$(top_builddir)/lib/libdvbsub/libdvbsub.a \
	$(top_builddir)/lib/libtuxtxt/libtuxtxt.a \
	$(top_builddir)/lib/libupnpclient/libtuxbox-upnpclient.a \
	$(top_builddir)/lib/libcoolstream/libcoolstream.a \
	@CURL_LIBS@ \
	@FREETYPE_LIBS@ \
	@PNG_LIBS@ \
	@MAD_LIBS@ \
	@ID3TAG_LIBS@ \
	@VORBISIDEC_LIBS@ \
	-ljpeg -lgif -lrt -lpthread -lblkid -ldl -lFLAC -lavformat -lavcodec -lavutil

if !ENABLE_GSTREAMER
neutrino_LDADD += $(top_builddir)/lib/libeplayer3/libeplayer3.a -lass
endif



# LCD display
if ENABLE_LCD
neutrino_LDADD += $(top_builddir)/lib/liblcddisplay/liblcddisplay.a
endif









	



