import os
from eyedb.test.command import run_java_command

subdir = "%s/examples/JavaBinding/generic/appli/basic" % (os.environ['top_builddir'])
run_java_command( 'Basic', args='person_j', classpath=[subdir])
