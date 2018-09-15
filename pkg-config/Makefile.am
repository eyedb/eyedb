pkgconfigdir=$(libdir)/pkgconfig
pkgconfig_DATA= eyedb.pc

EDIT = sed \
        -e 's,@bindir\@,$(bindir),g' \
        -e 's,@sbindir\@,$(sbindir),g' \
        -e 's,@EYEDB_ARCH_LIBS\@,$(EYEDB_ARCH_LIBS),g' \
        -e 's,@includedir\@,$(includedir),g' \
        -e 's,@libdir\@,$(libdir),g' \
        -e 's,@EYEDB_VERSION\@,$(EYEDB_VERSION),g' 

%.pc: Makefile $(srcdir)/%.pc.in
	rm -f $*.pc $*.pc.tmp
	$(EDIT) $(srcdir)/$*.pc.in >$*.pc.tmp
	mv $*.pc.tmp $*.pc

EXTRA_DIST=eyedb.pc.in

