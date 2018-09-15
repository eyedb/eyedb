import os
from eyedb.test.command import run_java_command

subdir = "%s/examples/JavaBinding/schema-oriented/store" % (os.environ['top_builddir'])
sharedir = "%s/examples/JavaBinding/schema-oriented/share/person.jar" % (os.environ['top_builddir'])
run_java_command( 'PersonTest', args=['person_j', 'toto'], classpath=[subdir,sharedir])
