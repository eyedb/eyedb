import os
from eyedb.test.command import run_java_command

subdir = "%s/examples/JavaBinding/generic/appli/schema" % (os.environ['top_builddir'])
run_java_command( 'SchemaDump', args='person_j', classpath=[subdir])
