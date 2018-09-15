import os
from eyedb.test.command import run_java_command

subdir = "%s/examples/JavaBinding/generic/appli/store" % (os.environ['top_builddir'])
run_java_command( 'Store', args=['person_j', 'toto', '42'], classpath=[subdir])
