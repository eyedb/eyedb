eyedbexamplesdir=$(datadir)/doc/$(PACKAGE)/examples/JavaBinding/generic/applet
dist_eyedbexamples_DATA= Test.java applet.html 
eyedbexamples_DATA=Makefile.eyedb-examples

JAVAROOT=$(top_builddir)/examples/JavaBinding/generic/applet
CLASSPATH_ENV= CLASSPATH=$(JAVAROOT):$(top_builddir)/src/java/eyedb.jar

check-local: class.stamp
class.stamp: Test.java
	$(CLASSPATH_ENV) $(JAVAC) -d $(JAVAROOT) $(AM_JAVACFLAGS) $(JAVACFLAGS) $?
	touch class.stamp

clean-local:
	-rm -f class.stamp *.class
