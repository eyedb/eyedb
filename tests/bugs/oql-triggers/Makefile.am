TESTS_ENVIRONMENT= $(SHELL) 

TESTS=\
test_init_schema.sh \
test_oql_update_trigger_crash.sh \
test_oql_update_trigger_same_values.sh \
test_oql_load_before_trigger_crash.sh \
test_oql_load_after_trigger_message.sh 

# Code snippet from autoconf documentation:
# http://www.gnu.org/software/autoconf/manual/autoconf.html#Installation-Directory-Variables
EDIT= sed \
        -e 's,@bindir[@],$(bindir),g' \
        -e 's,@exec_prefix[@],$(exec_prefix),g' \
        -e 's,@prefix[@],$(prefix),g' 

%.sh: Makefile $(srcdir)/%.sh.in
	rm -f $@ $@.tmp
	$(EDIT) '$(srcdir)/$@.in' >$@.tmp
	mv $@.tmp $@

EXTRA_DIST= \
schema.odl \
test_init_schema.sh.in \
test_oql_load_after_trigger_message.sh.in \
test_oql_load_before_trigger_crash.sh.in \
test_oql_update_trigger_crash.sh.in \
test_oql_update_trigger_same_values.sh.in
