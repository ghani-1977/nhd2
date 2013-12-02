INCLUDES = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/zapit/include/zapit \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/libconfigfile \
	-I$(top_srcdir)/lib/connection \
	-I$(top_srcdir)/lib/libeventserver \
	-I$(top_srcdir)/lib/libxmltree

if BOXTYPE_COOLSTREAM
INCLUDES += -I$(top_srcdir)/src/zapit/include \
	-I$(top_srcdir)/lib/libcoolstream
else
INCLUDES += -I$(top_srcdir)/lib/libdvbapi
endif

if ENABLE_LIBEPLAYER3
INCLUDES += -I$(top_srcdir)/lib/libeplayer3/include
endif

noinst_LIBRARIES = libzapit.a

libzapit_a_SOURCES = \
	bouquets.cpp channel.cpp dvbstring.cpp getservices.cpp \
	scan.cpp zapit.cpp cam.cpp ci.cpp frontend.cpp \
	descriptors.cpp nit.cpp pat.cpp pmt.cpp sdt.cpp

bin_PROGRAMS = pzapit

pzapit_SOURCES = \
	pzapit.cpp

pzapit_LDADD = \
	$(top_builddir)/src/zapit/lib/libzapitclient.a \
	$(top_builddir)/lib/connection/libtuxbox-connection.a
