from eyedb.test.command import run_simple_command
from eyedb.test.database import database_exists, database_delete, database_create
import os
import sys

dbname = 'index_test_db'

if database_exists( dbname, bindir = os.environ['bindir']):
    status = database_delete( dbname, bindir = os.environ['bindir'])
    if status != 0:
        sys.exit( status)

status = database_create( dbname, bindir = os.environ['bindir'])
if status != 0:
    sys.exit( status)

# fill an ODL schema into it
odl = "%s/eyedbodl -u -d %s --nocpp %s/person.odl" % (os.environ['bindir'], dbname, os.environ['srcdir'])
status = run_simple_command( odl, do_exit = False)
if status == 0:
    status = 77
sys.exit( status)
