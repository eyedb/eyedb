import os
from eyedb.test.command import run_java_command

subdir = "%s/examples/JavaBinding/generic/appli/query" % (os.environ['top_builddir'])
run_java_command( 'Query', args=['person_j','select Person'], classpath=[subdir])
