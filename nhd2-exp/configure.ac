AC_INIT(tuxbox-neutrino,1.0.1)
AM_INIT_AUTOMAKE(tuxbox-neutrino,1.0.1) 

TUXBOX_APPS
TUXBOX_APPS_DIRECTORY
TUXBOX_APPS_PKGCONFIG
TUXBOX_APPS_DVB
TUXBOX_APPS_DRIVER

TUXBOX_BOXTYPE

AC_PROG_CC
AC_PROG_CXX
AC_DISABLE_STATIC
AM_PROG_LIBTOOL


TUXBOX_APPS_LIB_CONFIG(CURL,curl-config)
TUXBOX_APPS_LIB_CONFIG(FREETYPE,freetype-config)
TUXBOX_APPS_LIB_PKGCONFIG(ID3TAG,id3tag)
TUXBOX_APPS_LIB_PKGCONFIG(MAD,mad)
TUXBOX_APPS_LIB_PKGCONFIG(VORBISIDEC,vorbisidec)
TUXBOX_APPS_LIB_PKGCONFIG(PNG,libpng)
TUXBOX_APPS_LIB_PKGCONFIG(AVCODEC,libavcodec)
TUXBOX_APPS_LIB_PKGCONFIG(AVUTIL,libavutil)
TUXBOX_APPS_LIB_PKGCONFIG(AVFORMAT,libavformat)

# keybaord
AC_ARG_ENABLE(keyboard-no-rc,
         [  --enable-keyboard-no-rc             enable keyboard control, disable rc control],
         [AC_DEFINE(KEYBOARD_INSTEAD_OF_REMOTE_CONTROL,1,[enable keyboard control, disable rc control])])

# restore prev mode
AC_ARG_ENABLE(restore-prev-mode,
         [  --enable-restore-prev-mode          enable return from graphics mode],
         [AC_DEFINE(RETURN_FROM_GRAPHICS_MODE,1,[enable return from graphics mode])])


#AC_SUBST(FREETYPE_CFLAGS)
#AC_SUBST(FREETYPE_LIBS)
#AC_SUBST(VORBISIDEC_CFLAGS)
#AC_SUBST(VORBISIDEC_LIBS)


# TEST_MENU
AC_ARG_ENABLE(testmenu,
	AS_HELP_STRING(--enable-testmenu,include Neutrino TEST_MENU support),
	,[enable_testmenu=no])
AM_CONDITIONAL(ENABLE_TEST_MENU,test "$enable_testmenu" = "yes")

if test "$enable_test" = "yes"; then
AC_DEFINE(ENABLE_TEST_MENU,1,[include test menu in gui])
fi

# PVR
AC_ARG_ENABLE(pvr,
	AS_HELP_STRING(--enable-pvr,enable playing TS throught dvr support),
	,[enable_pvr=no])
AM_CONDITIONAL(ENABLE_PVR,test "$enable_pvr" = "yes")

if test "$enable_pvr" = "yes"; then
AC_DEFINE(ENABLE_PVR,1,[enable playing TS throught dvr support])
fi

# libeplayer3
AC_ARG_ENABLE(libeplayer3,
	AS_HELP_STRING(--enable-libeplayer3,include Neutrino TDT libeplayer3 as player engine support),
	,[enable_libeplayer3=no])
AM_CONDITIONAL(ENABLE_LIBEPLAYER3,test "$enable_libeplayer3" = "yes")

if test "$enable_libeplayer3" = "yes"; then
AC_DEFINE(ENABLE_LIBEPLAYER3,1,[include libeplayer3 engine])
fi

# libass
AC_ARG_ENABLE(libass,
	AS_HELP_STRING(--enable-libass,include libass to avoid media subtilte support),
	,[enable_libass=no])
AM_CONDITIONAL(ENABLE_LIBASS,test "$enable_libass" = "yes")

if test "$enable_libass" = "yes"; then
AC_DEFINE(ENABLE_LIBASS,1,[include libass to avoid media subtilte support])
fi

# gstreamer
AC_ARG_ENABLE(gstreamer,
	AS_HELP_STRING(--enable-gstreamer,include enigma2 gstreamer as player engine support),
	,[enable_gstreamer=no])
AM_CONDITIONAL(ENABLE_GSTREAMER,test "$enable_gstreamer" = "yes")

if test "$enable_gstreamer" = "yes"; then
AC_DEFINE(ENABLE_GSTREAMER,1,[include gstreamer engine])
fi

# PCM soft decoder
AC_ARG_ENABLE(pcmsoftdecoder,
	AS_HELP_STRING(--enable-pcmsoftdecoder,include PCM Soft decoder as audio player support),
	,[enable_pcmsoftdecoder=no])
AM_CONDITIONAL(ENABLE_PCMSOFTDECODER,test "$enable_pcmsoftdecoder" = "yes")

if test "$enable_pcmsoftdecoder" = "yes"; then
AC_DEFINE(ENABLE_PCMSOFTDECODER,1,[include pcm soft decoder])
fi

# dsp
AC_ARG_ENABLE(dsp,
	AS_HELP_STRING(--enable-dsp,include DSP as audio player support),
	,[enable_dsp=no])
AM_CONDITIONAL(ENABLE_DSP,test "$enable_dsp" = "yes")

if test "$enable_dsp" = "yes"; then
AC_DEFINE(ENABLE_DSP,1,[include dsp as audio player])
fi


AC_OUTPUT([
Makefile
lib/Makefile
lib/libconfigfile/Makefile
lib/connection/Makefile
lib/libeventserver/Makefile
lib/libmd5sum/Makefile
lib/libnet/Makefile
lib/libxmltree/Makefile
lib/sectionsdclient/Makefile
lib/timerdclient/Makefile
lib/libtuxtxt/Makefile
lib/libdvbsub/Makefile
lib/libupnpclient/Makefile
lib/liblcddisplay/Makefile
lib/libcoolstream/Makefile
lib/libeplayer3/Makefile
src/nhttpd/Makefile
src/nhttpd/web/Makefile
src/nhttpd/web/images/Makefile
src/nhttpd/web/scripts/Makefile
src/nhttpd/web/languages/Makefile
src/nhttpd/tuxboxapi/Makefile
src/nhttpd/yhttpd_core/Makefile
src/nhttpd/yhttpd_mods/Makefile
src/Makefile
src/daemonc/Makefile
src/driver/pictureviewer/Makefile
src/driver/audiodec/Makefile
src/driver/Makefile
src/gui/Makefile
src/gui/bedit/Makefile
src/gui/widget/Makefile
src/system/Makefile
data/Makefile
data/fonts/Makefile
data/icons/Makefile
data/iso-codes/Makefile
data/inetradio/Makefile
data/locale/Makefile
data/config/Makefile
src/sectionsd/Makefile
src/timerd/Makefile
src/zapit/Makefile
src/zapit/lib/Makefile
src/zapit/src/Makefile
])

