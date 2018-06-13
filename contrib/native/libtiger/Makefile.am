SUBDIRS=.
if HAVE_DOXYGEN
SUBDIRS+=doc
endif

tigerincdir=$(includedir)/tiger
tigerpcdir=$(libdir)/pkgconfig

lib_LTLIBRARIES=lib/libtiger.la
tigerinc_HEADERS=include/tiger/tiger.h include/tiger/tiger_config.h
tigerpc_DATA=misc/pkgconfig/tiger.pc
noinst_DATA=misc/pkgconfig/tiger-uninstalled.pc

AM_LIBTOOLFLAGS=--silent

AM_CPPFLAGS=-I $(top_srcdir)/src -I $(top_srcdir)/include

lib_libtiger_la_SOURCES=\
  src/tiger.c \
  src/tiger_renderer.c \
  src/tiger_item.c \
  src/tiger_draw.c \
  src/tiger_util.c \
  src/tiger_bitmap.c \
  src/tiger_bitmap_cache.c \
  src/tiger_rectangle.c \
  src/tiger_pixel_format.c \
  src/tiger_debug.c

noinst_HEADERS=\
  src/tiger_internal.h \
  src/tiger_thread.h \
  src/tiger_mutex.h

lib_libtiger_la_CFLAGS=@CWARNFLAGS_FULL@ @CFLAGS_FORTIFY_SOURCE@ @CFLAGS_DEBUG@ @CFLAGS_PROFILE@ \
                       @PANGOCAIRO_CFLAGS@ @KATE_CFLAGS@ @PTHREAD_CFLAGS@
lib_libtiger_la_LIBADD=@PANGOCAIRO_LIBS@ @KATE_LIBS@ @PTHREAD_LIBS@
lib_libtiger_la_LDFLAGS=-version-info @LIBTIGER_SHARED_VERSION@ -no-undefined

am__tar = ${AMTAR} chof - --owner=0 --group=0 --exclude=CVS --exclude=.cvsignore "$$tardir"

check-local:
	@echo " Checking Tiger namespace"
	@! @NM@ -B lib/.libs/*.a lib/.libs/*.so \
	 | $(GREP) "^[0123456789abcdef]\{8\} [ABCDEFGHIJKLMOPQRSTUVWXYZ] [^\.]" \
	 | $(GREP) -v "^.\{11\}_" \
	 | $(GREP) -v "^.\{11\}tiger_" \
	 | $(GREP) -v "^.\{11\}kate_"
	@echo " Checking memory allocation routines"
	@! $(GREP) -EHn '[^_](malloc|realloc|free|calloc|memalign)\(' \
           $(top_srcdir)/src/*.[ch] $(top_srcdir)/include/tiger/*.h \
	 | $(GREP) -v "^include/tiger/tiger.h:#define tiger_"
	@echo " Checking forgotten debug traces"
	@! $(GREP) -EHn '[^sn]printf' \
	  $(top_srcdir)/src/*.[ch] $(top_srcdir)/include/tiger/*.h \
	| grep -v /src/tiger_debug.c: \
	| $(GREP) .


.PHONY: force-doc
force-doc:
	$(MAKE) -C doc force-doc

EXTRA_DIST=\
  README INSTALL AUTHORS COPYING THANKS ChangeLog \
  misc/pkgconfig/tiger.pc.in misc/pkgconfig/tiger-uninstalled.pc.in \
  examples/src/common.h examples/src/common.c examples/src/simple.c

