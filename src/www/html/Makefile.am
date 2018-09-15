htmldir=$(pkglibdir)/www
noinst_DATA=browser.html cbrowser.html

install-data-local:
	test -z "$(htmldir)" || $(mkinstalldirs) "$(DESTDIR)$(htmldir)"
	( \
		for f in $(noinst_DATA) ; do \
			test ! -f $(DESTDIR)$(htmldir)/$$f && $(INSTALL_DATA) $$f $(DESTDIR)$(htmldir)/$$f ; \
		done ; \
		exit 0 \
	)

EXTRA_DIST= browser.html cbrowser.html
