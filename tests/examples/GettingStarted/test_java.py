import os
from eyedb.test.command import run_java_command

subdir = "%s/examples/GettingStarted" % (os.environ['top_builddir'])
run_java_command( 'PersonTest', args='person_g', classpath=[subdir])


