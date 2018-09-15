eyedbexamplesdir=$(datadir)/doc/$(PACKAGE)/examples/JavaBinding/generic/appli/store
dist_eyedbexamples_DATA=Store.java
eyedbexamples_DATA= Makefile.eyedb-examples

JAVAROOT=$(top_builddir)/examples/JavaBinding/generic/appli/store
CLASSPATH_ENV= CLASSPATH=$(JAVAROOT):$(top_builddir)/src/java/eyedb.jar

check-local: class.stamp
class.stamp: Store.java
	$(CLASSPATH_ENV) $(JAVAC) -d $(JAVAROOT) $(AM_JAVACFLAGS) $(JAVACFLAGS) $?
	touch class.stamp

clean-local:
	-rm -f class.stamp *.class
