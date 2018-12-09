if COND_DOC
DOC_SRC_DIR=doc
else
DOC_SRC_DIR=
endif

# FD CHECK
#SUBDIRS=src include scripts install pkg-config examples tests $(DOC_SRC_DIR) 
SUBDIRS=src include scripts install pkg-config

ACLOCAL_AMFLAGS=-I m4

docdir=$(datadir)/doc/$(PACKAGE)
doc_DATA=AUTHORS COPYING COPYING.LESSER README

EXTRA_DIST= AUTHORS COPYING COPYING.LESSER ChangeLog README VERSION autogen.sh

# TAGS:
# 	find $(top_srcdir) \! -name '*~' \( -name '*.c' -o -name '*.h' -o -name '*.cc' -o -name '*.l' -o -name '*.y' -o -name '*.java' -o -name 'Makefile.am' -o \( -name '*.in' -a \! -name Makefile.in \) \) -print | etags -
# .PHONY: TAGS

doc:
	(cd doc && $(MAKE) $(AM_MAKEFLAGS) all)
	(cd examples && $(MAKE) $(AM_MAKEFLAGS) all)
.PHONY: doc

doc-install:
	(cd doc && $(MAKE) $(AM_MAKEFLAGS) install)
	(cd examples && $(MAKE) $(AM_MAKEFLAGS) install)
.PHONY: doc-install

doc-export:
	(cd doc && $(MAKE) $(AM_MAKEFLAGS) doc-export)
.PHONY: doc-export

doc-archive:
	(cd doc && $(MAKE) $(AM_MAKEFLAGS) doc-archive)
.PHONY: doc-archive

#check-ascii:
#	( \
#		cd $(top_srcdir) ; \
#		for f in `find . -name '*.java'` ; \
#		do \
#			echo $$f ; \
#			iconv --to-code=ASCII -c --output=- $$f | diff --brief - $$f ; \
#			if test $$? != 0 ; then echo $$f not ASCII; exit 1; fi ; \
#		done ; \
#		echo No non-ASCII java file found ; \
#	)

#dist-hook: check-ascii

AUTOMAKE_OPTIONS=dist-bzip2


# distcheck cannot work for eyedb, so we make our own
mydistcheck: dist
	bunzip2 -c $(distdir).tar.bz2 | $(am__untar)
	mkdir $(distdir)/_build
	mkdir $(distdir)/_inst
	dc_install_base=`$(am__cd) $(distdir)/_inst && pwd | sed -e 's,^[^:\\/]:[\\/],/,'` \
	  && dc_destdir="$${TMPDIR-/tmp}/am-dc-$$$$/" \
	  && cd $(distdir)/_build \
	  && ../configure --srcdir=.. --prefix="$$dc_install_base" $(DISTCHECK_CONFIGURE_FLAGS)
	cd $(distdir)/_build ; $(MAKE) $(AM_MAKEFLAGS)
	cd $(distdir)/_build ; $(MAKE) $(AM_MAKEFLAGS) install
	$(SHELL) $(distdir)/_inst/share/eyedb/tools/eyedb-postinstall.sh
	cd $(distdir)/_build ; $(MAKE) $(AM_MAKEFLAGS) check
