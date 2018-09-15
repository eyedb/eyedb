man1_MANS= \
eyedbadmin-collection.xml \
eyedbadmin-database.xml \
eyedbadmin-datafile.xml \
eyedbadmin-dataspace.xml \
eyedbadmin-index.xml \
eyedbadmin-user.xml \
eyedbadmin.1 \
eyedbjrun.1 \
eyedbodl.1 \
eyedb-options.1 \
eyedboql.1 

#man5_MANS= \
#eyedb-config.5

man7_MANS= \
eyedb.7

man8_MANS= \
eyedbctl.8 

XSLTPROC=@XSLTPROC@
XSLTPROCFLAGS=--novalid --nonet
MANPAGES_XSL_STYLESHEET=@MANPAGES_XSL_STYLESHEET_DIR@/docbook.xsl

%.1: $(srcdir)/%.xml
	$(XSLTPROC) $(XSLTPROCFLAGS) $(MANPAGES_XSL_STYLESHEET) $(srcdir)/$*.xml

%.5: $(srcdir)/%.xml
	$(XSLTPROC) $(XSLTPROCFLAGS) $(MANPAGES_XSL_STYLESHEET) $(srcdir)/$*.xml

%.7: $(srcdir)/%.xml
	$(XSLTPROC) $(XSLTPROCFLAGS) $(MANPAGES_XSL_STYLESHEET) $(srcdir)/$*.xml

%.8: $(srcdir)/%.xml
	$(XSLTPROC) $(XSLTPROCFLAGS) $(MANPAGES_XSL_STYLESHEET) $(srcdir)/$*.xml

EXTRA_DIST= \
$(subst .1,.xml,$(man1_MANS))\
$(subst .5,.xml,$(man5_MANS))\
$(subst .7,.xml,$(man7_MANS))\
$(subst .8,.xml,$(man8_MANS))

