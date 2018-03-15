bin_PROGRAMS = treadmill_mica

AM_CPPFLAGS = -I$(top_srcdir)/..

treadmill_mica_SOURCES = \
	services/mica/Connection.h \
	services/mica/MemcachedService.h \
	services/mica/Request.h \
	services/mica/Treadmill.cpp \
	services/mica/cityhash.h \
	services/mica/roundup.h \
	services/mica/utils.h \
	services/mica/memcpy.h \
	services/mica/zipf.h \
	services/mica/Workload.h

treadmill_mica_LDADD = \
	  $(top_builddir)/libtreadmill.a
