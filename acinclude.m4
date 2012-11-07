AC_DEFUN([TUXBOX_APPS],[
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

AC_GNU_SOURCE
AC_SYS_LARGEFILE

AC_ARG_WITH(target,
	[  --with-target=TARGET    target for compilation [[native,cdk]]],
	[TARGET="$withval"],[TARGET="native"])

AC_ARG_WITH(targetprefix,
	[  --with-targetprefix=PATH  prefix relative to target root (only applicable in cdk mode)],
	[targetprefix="$withval"],[targetprefix="NONE"])

AC_ARG_WITH(debug,
	[  --without-debug         disable debugging code],
	[DEBUG="$withval"],[DEBUG="yes"])

if test "$DEBUG" = "yes"; then
	DEBUG_CFLAGS="-g3 -ggdb"
	AC_DEFINE(DEBUG,1,[Enable debug messages])
fi

AC_MSG_CHECKING(target)

if test "$TARGET" = "native"; then
	AC_MSG_RESULT(native)

	if test "$CFLAGS" = "" -a "$CXXFLAGS" = ""; then
		CFLAGS="-Wall -O2 -pipe $DEBUG_CFLAGS"
		CXXFLAGS="-Wall -O2 -pipe $DEBUG_CFLAGS"
	fi
	if test "$prefix" = "NONE"; then
		prefix=/usr/local
	fi
	targetprefix=$prefix
elif test "$TARGET" = "cdk"; then
	AC_MSG_RESULT(cdk)

	if test "$CC" = "" -a "$CXX" = ""; then
		CC=sh4-linux-gcc CXX=sh4-linux-g++
	fi
	if test "$CFLAGS" = "" -a "$CXXFLAGS" = ""; then
		CFLAGS="-Wall -O2 -pipe $DEBUG_CFLAGS"
		CXXFLAGS="-Wall -O2 -pipe $DEBUG_CFLAGS"
	fi
	if test "$prefix" = "NONE"; then
		AC_MSG_ERROR(invalid prefix, you need to specify one in cdk mode)
	fi
	if test "$targetprefix" = "NONE"; then
		targetprefix=""
	fi
	if test "$host_alias" = ""; then
		cross_compiling=yes
		host_alias=sh4-linux
	fi
else
	AC_MSG_RESULT(none)
	AC_MSG_ERROR([invalid target $TARGET, choose on from native,cdk]);
fi

AC_CANONICAL_BUILD
AC_CANONICAL_HOST

check_path () {
	return $(perl -e "if(\"$1\"=~m#^/usr/(local/)?bin#){print \"0\"}else{print \"1\";}")
}

])

AC_DEFUN([TUXBOX_APPS_DIRECTORY_ONE],[
AC_ARG_WITH($1,[  $6$7 [[PREFIX$4$5]]],[
	_$2=$withval
	if test "$TARGET" = "cdk"; then
		$2=`eval echo "${targetprefix}$withval"`
	else
		$2=$withval
	fi
	TARGET_$2=${$2}
],[
	$2="\${$3}$5"
	if test "$TARGET" = "cdk"; then
		_$2=`eval echo "${target$3}$5"`
	else
		_$2=`eval echo "${$3}$5"`
	fi
	TARGET_$2=$_$2
])

dnl automake <= 1.6 don't support this
dnl AC_SUBST($2)
AC_DEFINE_UNQUOTED($2,"$_$2",$7)
AC_SUBST(TARGET_$2)
])

AC_DEFUN([TUXBOX_APPS_DIRECTORY],[
AC_REQUIRE([TUXBOX_APPS])

if test "$TARGET" = "cdk"; then
	datadir="\${prefix}/share"
	sysconfdir="\${prefix}/etc"
	localstatedir="\${prefix}/var"
	libdir="\${prefix}/lib"
	targetdatadir="\${targetprefix}/share"
	targetsysconfdir="\${targetprefix}/etc"
	targetlocalstatedir="\${targetprefix}/var"
	targetlibdir="\${targetprefix}/lib"
fi

TUXBOX_APPS_DIRECTORY_ONE(configdir,CONFIGDIR,localstatedir,/var,/tuxbox/config,
	[--with-configdir=PATH   ],[where to find the config files])

TUXBOX_APPS_DIRECTORY_ONE(datadir,DATADIR,datadir,/share,/tuxbox,
	[--with-datadir=PATH     ],[where to find data])

TUXBOX_APPS_DIRECTORY_ONE(fontdir,FONTDIR,datadir,/share,/fonts,
	[--with-fontdir=PATH     ],[where to find the fonts])

TUXBOX_APPS_DIRECTORY_ONE(gamesdir,GAMESDIR,localstatedir,/var,/tuxbox/games,
	[--with-gamesdir=PATH    ],[where games data is stored])

TUXBOX_APPS_DIRECTORY_ONE(libdir,LIBDIR,libdir,/lib,/tuxbox,
	[--with-libdir=PATH      ],[where to find the internal libs])

TUXBOX_APPS_DIRECTORY_ONE(plugindir,PLUGINDIR,localstatedir,/var,/tuxbox/plugins,
	[--with-plugindir=PATH   ],[where to find the plugins])
])

dnl automake <= 1.6 needs this specifications
AC_SUBST(CONFIGDIR)
AC_SUBST(DATADIR)
AC_SUBST(FONTDIR)
AC_SUBST(GAMESDIR)
AC_SUBST(LIBDIR)
AC_SUBST(PLUGINDIR)
dnl end workaround

AC_DEFUN([TUXBOX_APPS_ENDIAN],[
AC_CHECK_HEADERS(endian.h)
AC_C_BIGENDIAN
])

AC_DEFUN([TUXBOX_APPS_DRIVER],[
AC_ARG_WITH(driver,
	[  --with-driver=PATH      path for driver sources [[NONE]]],
	[DRIVER="$withval"],[DRIVER=""])

if test "$DRIVER"; then
	AC_SUBST(DRIVER)
	CPPFLAGS="$CPPFLAGS -I$DRIVER/include"
fi
])

AC_DEFUN([TUXBOX_APPS_DVB],[
AC_ARG_WITH(dvbincludes,
	[  --with-dvbincludes=PATH  path for dvb includes [[NONE]]],
	[DVBINCLUDES="$withval"],[DVBINCLUDES=""])

if test "$DVBINCLUDES"; then
	CPPFLAGS="$CPPFLAGS -I$DVBINCLUDES"
fi

AC_CHECK_HEADERS(ost/dmx.h,[
	DVB_API_VERSION=1
	AC_MSG_NOTICE([found dvb version 1])
])

if test -z "$DVB_API_VERSION"; then
AC_CHECK_HEADERS(linux/dvb/version.h,[
	AC_LANG_PREPROC_REQUIRE()
	AC_REQUIRE([AC_PROG_EGREP])
	AC_LANG_CONFTEST([AC_LANG_SOURCE([[
#include <linux/dvb/version.h>
version DVB_API_VERSION
	]])])
	DVB_API_VERSION=`(eval "$ac_cpp conftest.$ac_ext") 2>&AS_MESSAGE_LOG_FD | $EGREP "^version" | sed "s,version\ ,,"`
	rm -f conftest*

	AC_MSG_NOTICE([found dvb version $DVB_API_VERSION])
])
fi

if test "$DVB_API_VERSION"; then
	AC_DEFINE(HAVE_DVB,1,[Define to 1 if you have the dvb includes])
	AC_DEFINE_UNQUOTED(HAVE_DVB_API_VERSION,$DVB_API_VERSION,[Define to the version of the dvb api])
else
	AC_MSG_ERROR([can't find dvb headers])
fi
])

AC_DEFUN([_TUXBOX_APPS_LIB_CONFIG],[
AC_PATH_PROG($1_CONFIG,$2,no)
if test "$$1_CONFIG" != "no"; then
	if test "$TARGET" = "cdk" && check_path "$$1_CONFIG"; then
		AC_MSG_$3([could not find a suitable version of $2]);
	else
		$1_CFLAGS=$($$1_CONFIG --cflags)
		$1_LIBS=$($$1_CONFIG --libs)
	fi
fi

AC_SUBST($1_CFLAGS)
AC_SUBST($1_LIBS)
])

AC_DEFUN([TUXBOX_APPS_LIB_CONFIG],[
_TUXBOX_APPS_LIB_CONFIG($1,$2,ERROR)
if test "$$1_CONFIG" = "no"; then
	AC_MSG_ERROR([could not find $2]);
fi
])

AC_DEFUN([TUXBOX_APPS_LIB_CONFIG_CHECK],[
_TUXBOX_APPS_LIB_CONFIG($1,$2,WARN)
])

AC_DEFUN([TUXBOX_APPS_PKGCONFIG],[
AC_PATH_PROG(PKG_CONFIG, pkg-config,no)
if test "$PKG_CONFIG" = "no" ; then
	AC_MSG_ERROR([could not find pkg-config]);
fi
])

AC_DEFUN([_TUXBOX_APPS_LIB_PKGCONFIG],[
AC_REQUIRE([TUXBOX_APPS_PKGCONFIG])
AC_MSG_CHECKING(for package $2)
if PKG_CONFIG_PATH="${prefix}/lib/pkgconfig" $PKG_CONFIG --exists "$2" ; then
	AC_MSG_RESULT(yes)
	$1_CFLAGS=$(PKG_CONFIG_PATH="${prefix}/lib/pkgconfig" $PKG_CONFIG --cflags "$2")
	$1_LIBS=$(PKG_CONFIG_PATH="${prefix}/lib/pkgconfig" $PKG_CONFIG --libs "$2")
else
	AC_MSG_RESULT(no)
fi

AC_SUBST($1_CFLAGS)
AC_SUBST($1_LIBS)
])

AC_DEFUN([TUXBOX_APPS_LIB_PKGCONFIG],[
_TUXBOX_APPS_LIB_PKGCONFIG($1,$2)
if test -z "$$1_CFLAGS" ; then
	AC_MSG_ERROR([could not find package $2]);
fi
])

AC_DEFUN([TUXBOX_APPS_LIB_PKGCONFIG_CHECK],[
_TUXBOX_APPS_LIB_PKGCONFIG($1,$2)
])

AC_DEFUN([_TUXBOX_APPS_LIB_SYMBOL],[
AC_CHECK_LIB($2,$3,HAVE_$1="yes",HAVE_$1="no")
if test "$HAVE_$1" = "yes"; then
	$1_LIBS=-l$2
fi

AC_SUBST($1_LIBS)
])

AC_DEFUN([TUXBOX_APPS_LIB_SYMBOL],[
_TUXBOX_APPS_LIB_SYMBOL($1,$2,$3,ERROR)
if test "$HAVE_$1" = "no"; then
	AC_MSG_ERROR([could not find $2]);
fi
])

AC_DEFUN([TUXBOX_APPS_LIB_CONFIG_SYMBOL],[
_TUXBOX_APPS_LIB_SYMBOL($1,$2,$3,WARN)
])

AC_DEFUN([TUXBOX_APPS_GETTEXT],[
AC_PATH_PROG(MSGFMT, msgfmt, no)
AC_PATH_PROG(GMSGFMT, gmsgfmt, $MSGFMT)
AC_PATH_PROG(XGETTEXT, xgettext, no)
AC_PATH_PROG(MSGMERGE, msgmerge, no)

AC_MSG_CHECKING([whether NLS is requested])
AC_ARG_ENABLE(nls,
	[  --disable-nls           do not use Native Language Support],
	USE_NLS=$enableval, USE_NLS=yes)
AC_MSG_RESULT($USE_NLS)
AC_SUBST(USE_NLS)

if test "$USE_NLS" = "yes"; then
	AC_CACHE_CHECK([for GNU gettext in libc], gt_cv_func_gnugettext_libc,[
		AC_TRY_LINK([
			#include <libintl.h>
			#ifndef __GNU_GETTEXT_SUPPORTED_REVISION
			#define __GNU_GETTEXT_SUPPORTED_REVISION(major) ((major) == 0 ? 0 : -1)
			#endif
			extern int _nl_msg_cat_cntr;
			extern int *_nl_domain_bindings;
			],[
			bindtextdomain ("", "");
			return (int) gettext ("") + _nl_msg_cat_cntr + *_nl_domain_bindings;
			], gt_cv_func_gnugettext_libc=yes, gt_cv_func_gnugettext_libc=no
		)]
	)

	if test "$gt_cv_func_gnugettext_libc" = "yes"; then
		AC_DEFINE(ENABLE_NLS, 1, [Define to 1 if translation of program messages to the user's native language is requested.])
		gt_use_preinstalled_gnugettext=yes
	else
		USE_NLS=no
	fi
fi

if test -f "$srcdir/po/LINGUAS"; then
	ALL_LINGUAS=$(sed -e "/^#/d" "$srcdir/po/LINGUAS")
fi

POFILES=
GMOFILES=
UPDATEPOFILES=
DUMMYPOFILES=
for lang in $ALL_LINGUAS; do
	POFILES="$POFILES $srcdirpre$lang.po"
	GMOFILES="$GMOFILES $srcdirpre$lang.gmo"
	UPDATEPOFILES="$UPDATEPOFILES $lang.po-update"
	DUMMYPOFILES="$DUMMYPOFILES $lang.nop"
done
INST_LINGUAS=
if test -n "$ALL_LINGUAS"; then
	for presentlang in $ALL_LINGUAS; do
		useit=no
		if test -n "$LINGUAS"; then
			desiredlanguages="$LINGUAS"
		else
			desiredlanguages="$ALL_LINGUAS"
		fi
		for desiredlang in $desiredlanguages; do
			case "$desiredlang" in
				"$presentlang"*) useit=yes;;
			esac
		done
		if test $useit = yes; then
			INST_LINGUAS="$INST_LINGUAS $presentlang"
		fi
	done
fi
CATALOGS=
if test -n "$INST_LINGUAS"; then
	for lang in $INST_LINGUAS; do
		CATALOGS="$CATALOGS $lang.gmo"
	done
fi
AC_SUBST(POFILES)
AC_SUBST(GMOFILES)
AC_SUBST(UPDATEPOFILES)
AC_SUBST(DUMMYPOFILES)
AC_SUBST(CATALOGS)
])

dnl backward compatiblity
AC_DEFUN([AC_GNU_SOURCE],
[AH_VERBATIM([_GNU_SOURCE],
[/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# undef _GNU_SOURCE
#endif])dnl
AC_BEFORE([$0], [AC_COMPILE_IFELSE])dnl
AC_BEFORE([$0], [AC_RUN_IFELSE])dnl
AC_DEFINE([_GNU_SOURCE])
])

AC_DEFUN([AC_PROG_EGREP],
[AC_CACHE_CHECK([for egrep], [ac_cv_prog_egrep],
   [if echo a | (grep -E '(a|b)') >/dev/null 2>&1
    then ac_cv_prog_egrep='grep -E'
    else ac_cv_prog_egrep='egrep'
    fi])
 EGREP=$ac_cv_prog_egrep
 AC_SUBST([EGREP])
])

AC_DEFUN([TUXBOX_BOXTYPE],[

AC_ARG_WITH(boxtype,
	[  --with-boxtype          valid values: cuberevo, cuberevo_mini, cuberevo_mini2, cuberevo_mini_fta, cuberevo_250hd, cuberevo_2000hd, cuberevo_9500hd, gigablue, dreambox, xtrend, spark7162, ufs910,ufs912,ufs913,ufs922,ipbox55,ipbox99,ipbox9900,tf7700,fortis_hdbox,octagon1008,atevio7500,spark,hl101,hs7110,hs7810a,adb_box,whitebox,vip,homecast5101, vuplus],
	[case "${withval}" in
		cuberevo|cuberevo_mini|cuberevo_mini2|cuberevo_mini_fta|cuberevo_250hd|cuberevo_2000hd|cuberevo_9500hd|gigablue|dreambox|xtrend|spark7162|ufs910|ufs912|ufs913|ufs922|ipbox55|ipbox99|ipbox9900|tf7700|fortis_hdbox|octagon1008|atevio7500|spark|hl101|hs7110|hs7810a|adb_box|whitebox|vip|homecast5101|vuplus)
			BOXTYPE="$withval"
			;;
		*)
			AC_MSG_ERROR([unsupported value $withval for --with-boxtype])
			;;
	esac], [BOXTYPE="cuberevo_mini2"])


AC_SUBST(BOXTYPE)

AM_CONDITIONAL(BOXTYPE_CUBEREVO, test "$BOXTYPE" = "cuberevo")
AM_CONDITIONAL(BOXTYPE_CUBEREVO_MINI, test "$BOXTYPE" = "cuberevo_mini")
AM_CONDITIONAL(BOXTYPE_CUBEREVO_MINI2, test "$BOXTYPE" = "cuberevo_mini2")
AM_CONDITIONAL(BOXTYPE_CUBEREVO_MINI_FTA, test "$BOXTYPE" = "cuberevo_mini_fta")
AM_CONDITIONAL(BOXTYPE_CUBEREVO_250HD, test "$BOXTYPE" = "cuberevo_250hd")
AM_CONDITIONAL(BOXTYPE_CUBEREVO_2000HD, test "$BOXTYPE" = "cuberevo_2000hd")
AM_CONDITIONAL(BOXTYPE_CUBEREVO_9500HD, test "$BOXTYPE" = "cuberevo_9500HD")
AM_CONDITIONAL(BOXTYPE_GIGABLUE, test "$BOXTYPE" = "gigablue")
AM_CONDITIONAL(BOXTYPE_DREAMBOX, test "$BOXTYPE" = "dreambox")
AM_CONDITIONAL(BOXTYPE_XTREND, test "$BOXTYPE" = "xtrend")
AM_CONDITIONAL(BOXTYPE_SPARK7162, test "$BOXTYPE" = "spark7162")
AM_CONDITIONAL(BOXTYPE_UFS910, test "$BOXTYPE" = "ufs910")
AM_CONDITIONAL(BOXTYPE_UFS912, test "$BOXTYPE" = "ufs912")
AM_CONDITIONAL(BOXTYPE_UFS913, test "$BOXTYPE" = "ufs913")
AM_CONDITIONAL(BOXTYPE_UFS922, test "$BOXTYPE" = "ufs922")
AM_CONDITIONAL(BOXTYPE_IPBOX55, test "$BOXTYPE" = "ipbox55")
AM_CONDITIONAL(BOXTYPE_IPBOX99, test "$BOXTYPE" = "ipbox99")
AM_CONDITIONAL(BOXTYPE_IPBOX9900, test "$BOXTYPE" = "ipbox9900")
AM_CONDITIONAL(BOXTYPE_TF7700, test "$BOXTYPE" = "tf7700")
AM_CONDITIONAL(BOXTYPE_FORTIS_HDBOX, test "$BOXTYPE" = "fortis_hdbox")
AM_CONDITIONAL(BOXTYPE_OCTAGON1008, test "$BOXTYPE" = "octagon1008")
AM_CONDITIONAL(BOXTYPE_ATEVIO7500, test "$BOXTYPE" = "atevio7500")
AM_CONDITIONAL(BOXTYPE_SPARK, test "$BOXTYPE" = "spark")
AM_CONDITIONAL(BOXTYPE_HL101, test "$BOXTYPE" = "hl101")
AM_CONDITIONAL(BOXTYPE_HS7110, test "$BOXTYPE" = "hs7110")
AM_CONDITIONAL(BOXTYPE_HS7810A, test "$BOXTYPE" = "hs7810a")
AM_CONDITIONAL(BOXTYPE_ADB_BOX, test "$BOXTYPE" = "adb_box")
AM_CONDITIONAL(BOXTYPE_WHITEBOX, test "$BOXTYPE" = "whitebox")
AM_CONDITIONAL(BOXTYPE_VIP, test "$BOXTYPE" = "vip")
AM_CONDITIONAL(BOXTYPE_HOMECAST5101, test "$BOXTYPE" = "homecast5101")
AM_CONDITIONAL(BOXTYPE_VUPLUS, test "$BOXTYPE" = "vuplus")

if test "$BOXTYPE" = "cuberevo"; then
	AC_DEFINE(PLATFORM_CUBEREVO, 1, [building for cuberevo])
elif test "$BOXTYPE" = "cuberevo_mini"; then
	AC_DEFINE(PLATFORM_CUBEREVO_MINI, 1, [building for cuberevo_mini])
elif test "$BOXTYPE" = "cuberevo_mini2"; then
	AC_DEFINE(PLATFORM_CUBEREVO_MINI2, 1, [building for cuberevo_mini2])
elif test "$BOXTYPE" = "cuberevo_mini_fta"; then
	AC_DEFINE(PLATFORM_CUBEREVO_MINI_FTA, 1, [building for cuberevo_mini_fta])
elif test "$BOXTYPE" = "cuberevo_250hd"; then
	AC_DEFINE(PLATFORM_CUBEREVO_250HD, 1, [building for cuberevo_250hd])
elif test "$BOXTYPE" = "cuberevo_2000hd"; then
	AC_DEFINE(PLATFORM_CUBEREVO_2000HD, 1, [building for cuberevo_2000hd])
elif test "$BOXTYPE" = "cuberevo_9500hd"; then
	AC_DEFINE(PLATFORM_CUBEREVO_9500HD, 1, [building for cuberevo_9500hd])
elif test "$BOXTYPE" = "gigablue"; then
	AC_DEFINE(PLATFORM_GIGABLUE, 1, [building for gigablue])
elif test "$BOXTYPE" = "dreambox"; then
	AC_DEFINE(PLATFORM_DREAMBOX, 1, [building for dreambox])
elif test "$BOXTYPE" = "xtrend"; then
	AC_DEFINE(PLATFORM_XTREND, 1, [building for xtrend])
elif test "$BOXTYPE" = "spark7162"; then
	AC_DEFINE(PLATFORM_SPARK7162, 1, [building for fulan spark 7162])
elif test "$BOXTYPE" = "ufs910"; then
	AC_DEFINE(PLATFORM_UFS910, 1, [building for a ufs910])
elif test "$BOXTYPE" = "ufs912"; then
	AC_DEFINE(PLATFORM_UFS912, 1, [building for a ufs912])
elif test "$BOXTYPE" = "ufs913"; then
	AC_DEFINE(PLATFORM_UFS913, 1, [building for a ufs913])
elif test "$BOXTYPE" = "ufs922"; then
	AC_DEFINE(PLATFORM_UFS922, 1, [building for an ufs922])
elif test "$BOXTYPE" = "ipbox55"; then
	AC_DEFINE(PLATFORM_IPBOX55, 1, [building for a ipbox55])
elif test "$BOXTYPE" = "ipbox99"; then
	AC_DEFINE(PLATFORM_IPBOX99, 1, [building for a ipbox99])
elif test "$BOXTYPE" = "ipbox9900"; then
	AC_DEFINE(PLATFORM_IPBOX9900, 1, [building for a ipbox9900])
elif test "$BOXTYPE" = "tf7700"; then
	AC_DEFINE(PLATFORM_TF7700, 1, [building for a tf7700])
elif test "$BOXTYPE" = "fortis_hdbox"; then
	AC_DEFINE(PLATFORM_FORTIS_HDBOX, 1, [building for a fortis_hdbox])
elif test "$BOXTYPE" = "octagon1008"; then
	AC_DEFINE(PLATFORM_OCTAGON1008, 1, [building for a octagon1008])
elif test "$BOXTYPE" = "atevio7500"; then
	AC_DEFINE(PLATFORM_ATEVIO7500, 1, [building for a atevio7500])
elif test "$BOXTYPE" = "spark"; then
	AC_DEFINE(PLATFORM_SPARK, 1, [building for an spark])
elif test "$BOXTYPE" = "hl101"; then
	AC_DEFINE(PLATFORM_HL101, 1, [building for a hl101])
elif test "$BOXTYPE" = "hs7110"; then
	AC_DEFINE(PLATFORM_HS7110, 1, [building for a hs7110])
elif test "$BOXTYPE" = "hs7810a"; then
	AC_DEFINE(PLATFORM_HS7810A, 1, [building for a hs7810a])
elif test "$BOXTYPE" = "adb_box"; then
	AC_DEFINE(PLATFORM_ADB_BOX, 1, [building for a adb_box])
elif test "$BOXTYPE" = "whitebox"; then
	AC_DEFINE(PLATFORM_WHITEBOX, 1, [building for a whitebox])
elif test "$BOXTYPE" = "vip"; then
	AC_DEFINE(PLATFORM_VIP, 1, [building for an vip])
elif test "$BOXTYPE" = "homecast5101"; then
	AC_DEFINE(PLATFORM_HOMECAST5101, 1, [building for a homecast5101])
elif test "$BOXTYPE" = "vuplus"; then
	AC_DEFINE(PLATFORM_VUPLUS, 1, [building for a vuplus])
fi
])


