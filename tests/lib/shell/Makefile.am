EDIT = sed \
        -e 's,@bindir\@,$(bindir),g' \
        -e 's,@sbindir\@,$(sbindir),g' \
        -e 's,@exec_prefix\@,$(exec_prefix),g' \
        -e 's,@libdir\@,$(libdir),g' \
        -e 's,@top_builddir[@],$(top_builddir),g' \
        -e 's,@abs_top_builddir[@],$(abs_top_builddir),g' \
        -e 's,@eyedb_databasedir\@,$(eyedb_databasedir),g' \
        -e 's,@eyedb_pipedir\@,$(eyedb_pipedir),g' \
        -e 's,@eyedb_tmpdir\@,$(eyedb_tmpdir),g' \
        -e 's,@prefix\@,$(prefix),g' \
        -e 's,@sysconfdir\@,$(sysconfdir),g' \
        -e 's,@EYEDBD_USER\@,$(EYEDBD_USER),g'

%: Makefile $(srcdir)/%.in
	rm -f $@ $@.tmp
	$(EDIT) '$(srcdir)/$@.in' >$@.tmp
	mv $@.tmp $@

nodist_noinst_SCRIPTS= \
eyedb-functions.sh

EXTRA_DIST=eyedb-functions.sh.in
