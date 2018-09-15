if COND_DOXYGEN
DOXYGEN_DIR= doxygen 
DOXYGEN_EXPORT_DIR=api/C++
else
DOXYGEN_DIR= 
DOXYGEN_EXPORT_DIR= 
endif

if COND_JAVADOC
JAVADOC_DIR= javadoc
JAVADOC_EXPORT_DIR= api/java
else
JAVADOC_DIR= 
JAVADOC_EXPORT_DIR= 
endif

if COND_DOCBOOK_MANPAGES
MANPAGES_DIR= manpages 
#MANPAGES_EXPORT_DIR= manpages 
else
MANPAGES_DIR= 
#MANPAGES_EXPORT_DIR= 
endif

if COND_LATEX
MANUAL_EXPORT_DIR= manual/pdf
else
MANUAL_EXPORT_DIR= 
endif

if COND_LATEX2HTML
HTML_MANUAL_EXPORT_DIR= manual/html
else
HTML_MANUAL_EXPORT_DIR= 
endif

if COND_DBLATEX
MANUAL_EXPORT_DIR= manual/pdf
else
MANUAL_EXPORT_DIR= 
endif

EXAMPLES_EXPORT_DIR=examples

SUBDIRS=$(DOXYGEN_DIR) $(JAVADOC_DIR) $(MANPAGES_DIR) manual images release-notes
DIST_SUBDIRS=doxygen javadoc manpages manual images release-notes

htmldir=$(datadir)/doc/$(PACKAGE)
dist_html_DATA=index.html eyedbdoc.css

if COND_DOC
DOC_EXPORT_DIRS= $(DOXYGEN_EXPORT_DIR) $() $(JAVADOC_EXPORT_DIR) $(MANPAGES_EXPORT_DIR) $(MANUAL_EXPORT_DIR) $(HTML_MANUAL_EXPORT_DIR) $(EXAMPLES_EXPORT_DIR)
endif

doc-export-check:
if !COND_DOC
	echo "*** Error: documentation building must be enabled to export documentation"
	exit 1
endif
if !COND_DOXYGEN
	echo "*** Error: C++ documentation building using Doxygen must be enabled to export documentation"
	exit 1
endif
if !COND_JAVADOC
	echo "*** Error: Java documentation building using javadoc must be enabled to export documentation"
	exit 1
endif
if !COND_DOCBOOK_MANPAGES
	echo "*** Error: man pages building using docbook must be enabled to export documentation"
	exit 1
endif
if !COND_LATEX
	echo "*** Error: PDF manuals building using Latex must be enabled to export documentation"
	exit 1
endif
if !COND_LATEX2HTML
	echo "*** Error: HTML manuals building using latex2html must be enabled to export documentation"
	exit 1
endif
	if test -z "$(EYEDB_WEB_HOST)" ; then \
		echo "*** Error: EYEDB_WEB_HOST must be defined (make EYEDB_WEB_HOST=some.where)" ; \
		exit 1 ; \
	fi
	for d in $(DOC_EXPORT_DIRS) ; do \
		if test ! -d $(datadir)/doc/$(PACKAGE)/$$d ; then \
			echo "*** Error: directory $$d missing" ; \
			exit 1 ; \
		fi ; \
	done

doc-export: doc-export-check install
	ssh $(EYEDB_WEB_HOST) cd eyedb.org/www/wordpress/wp-content/uploads/documentation \; rm -rf $(DOC_EXPORT_DIRS)
	cd $(datadir)/doc/$(PACKAGE) ; tar czf - $(DOC_EXPORT_DIRS) | ssh $(EYEDB_WEB_HOST) cd eyedb.org/www/wordpress/wp-content/uploads/documentation \; tar xvzf -

doc-archive: install
	-rm -rf /var/tmp/eyedb-doc-$(EYEDB_VERSION)
	cp -r $(datadir)/doc/$(PACKAGE) /var/tmp/eyedb-doc-$(EYEDB_VERSION)
	tar cvzf eyedb-doc-$(EYEDB_VERSION).tar.gz -C /var/tmp eyedb-doc-$(EYEDB_VERSION)
	tar cvjf eyedb-doc-$(EYEDB_VERSION).tar.bz2 -C /var/tmp eyedb-doc-$(EYEDB_VERSION)
	-rm -rf /var/tmp/eyedb-doc-$(EYEDB_VERSION)

