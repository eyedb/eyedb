JAVA_FILES= Database.java O0.java O1_1_1.java O1_1.java O1.java O2.java O3.java O4.java O5.java

SRCDIR_JAVA_FILES=$(addprefix $(top_srcdir)/tests/eyedb/dataspace/dspsch/,$(JAVA_FILES))

JAVAROOT=$(top_builddir)/tests/eyedb/dataspace
CLASSPATH_ENV= CLASSPATH=$(JAVAROOT):$(top_srcdir)/tests/eyedb/dataspace:$(top_builddir)/src/java/eyedb.jar

check-local: class.stamp
class.stamp: $(SRCDIR_JAVA_FILES)
	$(CLASSPATH_ENV) $(JAVAC) -d $(JAVAROOT) $(AM_JAVACFLAGS) $(JAVACFLAGS) $?
	touch class.stamp

clean-local:
	-rm -f class.stamp *.class

#
# ODL to Java translation
#
EYEDBODL_exe=$(bindir)/eyedbodl

EYEDBODL = $(EYEDBODL_exe) --output-dir=$(top_srcdir)/tests/eyedb/dataspace/dspsch
EYEDBODLFLAGS = --gencode=Java --no-rootclass

$(SRCDIR_JAVA_FILES) : $(top_srcdir)/tests/eyedb/dataspace/dspsch.odl
	@if test -x $(EYEDBODL_exe) ; then \
		echo $(EYEDBODL) $(EYEDBODLFLAGS) $< ; \$(EYEDBODL) $(EYEDBODLFLAGS) $< ; \
	else \
		echo Warning: $(EYEDBODL) not found, using $*.java old Java code; touch $*.java ; \
	fi

