eyedbexamplesdir=$(datadir)/doc/$(PACKAGE)/examples/JavaBinding/generic/appli/collections
dist_eyedbexamples_DATA=Collections.java
eyedbexamples_DATA=Makefile.eyedb-examples

JAVAROOT=$(top_builddir)/examples/JavaBinding/generic/appli/collections
CLASSPATH_ENV= CLASSPATH=$(JAVAROOT):$(top_builddir)/src/java/eyedb.jar

check-local: class.stamp
class.stamp: Collections.java
	$(CLASSPATH_ENV) $(JAVAC) -d $(JAVAROOT) $(AM_JAVACFLAGS) $(JAVACFLAGS) $?
	touch class.stamp

clean-local:
	-rm -f class.stamp *.class
