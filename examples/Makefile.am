if COND_JAVA
JAVA_SRC_DIR= JavaBinding
else
JAVA_SRC_DIR= 
endif

SUBDIRS=GettingStarted C++Binding $(JAVA_SRC_DIR) oql

pkgdata_DATA=Makefile.eyedb

eyedbexamplesdir=$(datadir)/doc/$(PACKAGE)/examples
dist_eyedbexamples_DATA=README
eyedbexamples_DATA= Makefile.eyedb-examples 

EYEDB_ARCH_CPPFLAGS=@EYEDB_ARCH_CPPFLAGS@
EYEDB_ARCH_CXXFLAGS=@EYEDB_ARCH_CXXFLAGS@
EYEDB_ARCH_SOFLAGS=@EYEDB_ARCH_SOFLAGS@
EYEDB_ARCH_LIBS=@EYEDB_ARCH_LIBS@

EDIT = sed \
        -e 's,@bindir\@,$(bindir),g' \
        -e 's,@includedir\@,$(includedir),g' \
        -e 's,@exec_prefix\@,$(exec_prefix),g' \
        -e 's,@datadir\@,$(pkgdatadir),g' \
        -e 's,@libdir\@,$(libdir),g' \
        -e 's,@localstatedir\@,$(localstatedir),g' \
        -e 's,@prefix\@,$(prefix),g' \
        -e 's,@sysconfdir\@,$(sysconfdir),g' \
        -e 's,@EYEDB_ARCH_LIBS\@,$(EYEDB_ARCH_LIBS),g' \
	-e 's,@EYEDB_ARCH_CPPFLAGS\@,$(EYEDB_ARCH_CPPFLAGS),g' \
	-e 's,@EYEDB_ARCH_CXXFLAGS\@,$(EYEDB_ARCH_CXXFLAGS),g' \
	-e 's,@EYEDB_ARCH_SOFLAGS\@,$(EYEDB_ARCH_SOFLAGS),g'

Makefile.eyedb: Makefile $(srcdir)/Makefile.eyedb.in
	rm -f $@ $@.tmp
	$(EDIT) $(srcdir)/$@.in >$@.tmp
	mv $@.tmp $@

install-data-hook:
	cd $(DESTDIR)$(eyedbexamplesdir) ; \
	for f in `find . -name Makefile.eyedb-examples` ; \
	do \
		mv $$f `dirname $$f`/Makefile ; \
	done

EXTRA_DIST= Makefile.eyedb.in
