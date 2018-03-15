bin_PROGRAMS = treadmill_mica

AM_CPPFLAGS = -I$(top_srcdir)/..

treadmill_mica_SOURCES = \
	 Connection.h \
	 MemcachedService.h \
	 Request.h \
	 Treadmill.cpp \
	 cityhash.h \
	 roundup.h \
	 utils.h \
	 memcpy.h \
	 zipf.h \
	 Workload.h

treadmill_mica_LDADD = \
	  $(top_builddir)/libtreadmill.a
