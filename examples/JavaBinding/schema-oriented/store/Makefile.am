eyedbexamplesdir=$(datadir)/doc/$(PACKAGE)/examples/JavaBinding/schema-oriented/store
dist_eyedbexamples_DATA= PersonTest.java
eyedbexamples_DATA=  Makefile.eyedb-examples

JAVAROOT=$(top_builddir)/examples/JavaBinding/schema-oriented/store
CLASSPATH_ENV= CLASSPATH=$(JAVAROOT):$(top_builddir)/src/java/eyedb.jar:$(top_builddir)/examples/JavaBinding/schema-oriented/share/person.jar:$(top_builddir)/examples/JavaBinding/schema-oriented/store

check-local: class.stamp
class.stamp: PersonTest.java 
	$(CLASSPATH_ENV) $(JAVAC) -d $(JAVAROOT) $(AM_JAVACFLAGS) $(JAVACFLAGS) $?
	touch class.stamp

clean-local:
	-rm -f class.stamp *.class
