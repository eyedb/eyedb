noinst_htmldir=$(datadir)/doc/$(PACKAGE)/api/C++
noinst_html_DATA= html/index.html

DOXYGEN=@DOXYGEN@

CPP_FILES= \
$(wildcard $(top_srcdir)/src/eyedb/*.cc) \
$(wildcard $(top_srcdir)/src/eyedb/*.h) \
$(wildcard $(top_srcdir)/src/eyedbsm/*.cc) \
$(wildcard $(top_srcdir)/src/eyedbsm/*.h) \
$(wildcard $(top_srcdir)/src/lib/*.cc) \
$(wildcard $(top_srcdir)/src/lib/*.h) 

$(noinst_html_DATA): eyedb-doxygen.cfg $(CPP_FILES)
	$(DOXYGEN) eyedb-doxygen.cfg
	for f in html/*.html ; \
	do \
		sed -e 's/ Eyedb/ eyedb/g' -e 's/>Eyedb/>eyedb/g' -e 's/ Eyedbsm/ eyedbsm/g' -e 's/>Eyedbsm/>eyedbsm/g' -e 's/ Eyedblib/ eyedblib/g' -e 's/>Eyedblib/>eyedblib/g' $$f > $$f.out ; mv $$f.out $$f ; \
	done

install-data-hook:
	test -z "$(noinst_htmldir)" || $(mkinstalldirs) "$(DESTDIR)$(noinst_htmldir)"
	( cd html ; \
	  for f in *.html *.css *.png ; do \
	    $(INSTALL_DATA) $$f "$(DESTDIR)$(noinst_htmldir)/$$f" ; \
	  done )

EXTRA_DIST=eyedb-doxygen.cfg
