noinst_htmldir=$(datadir)/doc/$(PACKAGE)/api/java
noinst_html_DATA= index.html

JAVADOC=@JAVADOC@
JAVADOC_FLAGS=-public -sourcepath $(top_srcdir)/src/java/src

JAVA_FILES= $(wildcard $(top_srcdir)/src/java/src/org/eyedb/*.java)

$(noinst_html_DATA): $(JAVA_FILES)
	$(JAVADOC) $(JAVADOC_FLAGS) org.eyedb

install-data-hook:
	test -z "$(noinst_htmldir)" || $(mkinstalldirs) "$(DESTDIR)$(noinst_htmldir)"
	for f in * ; do \
		test -f $$f && $(INSTALL_DATA) $$f "$(DESTDIR)$(noinst_htmldir)/$$f" ; \
	done
	for dir in resources org org/eyedb org/eyedb/dbm org/eyedb/syscls org/eyedb/utils; do \
		test -z "$(noinst_htmldir)/$$dir" || \
			$(mkinstalldirs) "$(DESTDIR)$(noinst_htmldir)/$$dir" ; \
		if test -d $$dir ; then \
			( \
				cd $$dir ; \
				for f in * ; do \
					test -f $$f && $(INSTALL_DATA) $$f "$(DESTDIR)$(noinst_htmldir)/$$dir/$$f" ; \
		  		done ; \
			) ; \
		fi \
	done
