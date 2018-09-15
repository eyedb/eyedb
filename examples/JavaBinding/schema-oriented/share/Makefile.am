eyedbexamplesdir=$(datadir)/doc/$(PACKAGE)/examples/JavaBinding/schema-oriented/share
dist_eyedbexamples_DATA= schema.odl 
nodist_eyedbexamples_DATA= init.sh
eyedbexamples_DATA=  Makefile.eyedb-examples

SUBDIRS=person

JAR=@JAR@
JARFILE=person.jar
CLEANFILES=$(JARFILE)

# Code snippet from autoconf documentation:
# http://www.gnu.org/software/autoconf/manual/autoconf.html#Installation-Directory-Variables
EDIT= sed \
        -e 's,@bindir[@],$(bindir),g' \
        -e 's,@exec_prefix[@],$(exec_prefix),g' \
        -e 's,@prefix[@],$(prefix),g' 

$(JARFILE): person/class.stamp
	$(JAR) -cvf $(JARFILE) `find person -name "*.class"`;

check-local: $(JARFILE)

%.sh: Makefile $(srcdir)/%.sh.in
	rm -f $@ $@.tmp
	$(EDIT) '$(srcdir)/$@.in' >$@.tmp
	mv $@.tmp $@

init: init.sh
	sh init.sh $(srcdir)/schema.odl
.PHONY: init

EXTRA_DIST=init.sh.in
