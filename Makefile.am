ACLOCAL_AMFLAGS = -I m4 --install

dist_doc_DATA = AUTHORS COPYING README.md

bin_PROGRAMS = odkrun

convlib_sources = lib/memreg.c lib/memreg.h \
		  lib/sbuffer.c lib/sbuffer.h \
		  lib/xmem.c lib/xmem.h

odkrun_SOURCES = src/odkrun.c \
		 src/procutil.c src/procutil.h \
		 src/runner.c src/runner.h \
		 $(convlib_sources)

AM_CPPFLAGS = -I$(top_srcdir)/lib

DEFS = -DFAIL_ON_ENOMEM @DEFS@

LDADD = $(LIBOBJS)
