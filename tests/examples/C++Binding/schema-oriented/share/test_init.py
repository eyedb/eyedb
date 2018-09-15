import os
import sys
from eyedb.test.command import run_simple_command
from eyedb.test.database import database_exists, database_delete

if database_exists( 'person_c', bindir = os.environ['bindir']):
    status = database_delete( 'person_c', bindir = os.environ['bindir'])
    if status != 0:
        sys.exit( status)

# run the init script
initscript= "sh -x %s/examples/C++Binding/schema-oriented/share/init.sh %s/examples/C++Binding/schema-oriented/share/schema.odl" % (os.environ['top_builddir'],os.environ['top_srcdir']) 
status = run_simple_command( initscript)
print
sys.exit( status)
