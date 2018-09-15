SUBDIRS=src

jardir=$(pkglibdir)/java
JARFILE=eyedb.jar
jar_DATA = $(JARFILE)

STAMP_FILE=classdist_package.stamp
JAR_DEPS=\
./src/org/eyedb/dbm/$(STAMP_FILE) \
./src/org/eyedb/syscls/$(STAMP_FILE) \
./src/org/eyedb/utils/$(STAMP_FILE) \
./src/org/eyedb/$(STAMP_FILE)

CLEANFILES=$(JARFILE)

JAR=@JAR@

$(JARFILE): $(JAR_DEPS)
	cd src ; $(JAR) -cvf ../$(JARFILE) `find . -name "*.class"`;
