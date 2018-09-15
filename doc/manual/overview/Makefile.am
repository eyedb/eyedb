DIST_SUBDIRS=figures

MANUAL=Overview

LATEX= TEXINPUTS=.:$(srcdir):$(top_srcdir)/doc/manual/latex-common:$(top_builddir)/doc/manual/latex-common: @LATEX@
LATEX_INPUTS=$(top_srcdir)/doc/manual/latex-common/start.tex $(top_srcdir)/doc/manual/latex-common/title.tex $(top_builddir)/doc/manual/latex-common/version.tex
DVIPS= TEXPICTS=$(srcdir) @DVIPS@
DVIPSFLAGS= -f -Ppdf -G0
PS2PDF=@PS2PDF@
PS2PDFFLAGS= -dPDFsettings=/prepress 
LATEX2HTML=@LATEX2HTML@
LATEX2HTMLFLAGS=-toc_depth 6 -top_navigation -verbosity 0 -local_icons -info '' -address 'EyeDB manual' -init_file $(top_builddir)/doc/manual/latex-common/latex2html-init 

manualdir=$(datadir)/doc/$(PACKAGE)/manual/pdf
PDF_PREFIX=EyeDB
manual_DATA=$(PDF_PREFIX)$(MANUAL).pdf

noinst_htmldir=$(datadir)/doc/$(PACKAGE)/manual/html
if COND_LATEX2HTML
noinst_html_DATA= html/$(MANUAL)/index.html
endif

%.dvi: %.tex
	$(LATEX) $<
	$(LATEX) $<
	if grep 'There were undefined references' $*.log > /dev/null ; then $(LATEX) $<; fi
.PRECIOUS: %.dvi

%.ps: %.dvi
	$(DVIPS) $(DVIPSFLAGS) $< -o $@

$(PDF_PREFIX)%.pdf: %.ps
	$(PS2PDF) $(PS2PDFFLAGS) $< $@

$(MANUAL).dvi: $(LATEX_INPUTS)

html/$(MANUAL)/index.html: $(LATEX_INPUTS)
if COND_LATEX2HTML
install-data-hook:
	test -z "$(noinst_htmldir)" || $(mkinstalldirs) "$(DESTDIR)$(noinst_htmldir)"
	-rm "$(DESTDIR)$(noinst_htmldir)/index.html"
	test -z "$(noinst_htmldir)/$(MANUAL)" || $(mkinstalldirs) "$(DESTDIR)$(noinst_htmldir)/$(MANUAL)" ; \
	(cd html/$(MANUAL) ; \
		for f in *.html *.css *.png ; do \
			$(INSTALL_DATA) $$f "$(DESTDIR)$(noinst_htmldir)/$(MANUAL)/$$f" ; \
		done ; \
	)
else
endif

html/Overview/index.html: Overview.tex
	-test ! -d html && mkdir html ; test ! -d html/Overview && mkdir html/Overview
	$(LATEX2HTML) $(LATEX2HTMLFLAGS) -split +2 -dir html/Overview $<

EXTRA_DIST=\
$(MANUAL).tex 

clean-local:
	rm -f *.pdf *.ps *.dvi *.aux *.idx *.log *.toc
	rm -f html/*/*
