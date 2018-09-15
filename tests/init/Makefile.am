TESTS_ENVIRONMENT= $(SHELL) 

TESTS=\
test_init_dbm.sh 

# Code snippet from autoconf documentation:
# http://www.gnu.org/software/autoconf/manual/autoconf.html#Installation-Directory-Variables
EDIT= sed \
        -e 's,@bindir[@],$(bindir),g' \
        -e 's,@exec_prefix[@],$(exec_prefix),g' \
        -e 's,@prefix[@],$(prefix),g' \
        -e 's,@top_builddir[@],$(top_builddir),g' \
        -e 's,@srcdir[@],$(srcdir),g' 

%.sh: Makefile $(srcdir)/%.sh.in
	rm -f $@ $@.tmp
	$(EDIT) '$(srcdir)/$@.in' >$@.tmp
	mv $@.tmp $@

EXTRA_DIST=\
test_init_dbm.sh.in 
