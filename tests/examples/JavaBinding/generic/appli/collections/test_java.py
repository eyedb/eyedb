import os
from eyedb.test.command import run_java_command

subdir = "%s/examples/JavaBinding/generic/appli/collections" % (os.environ['top_builddir'])
run_java_command( 'Collections', args=['person_j', 'toto'], classpath=[subdir])
