from eyedb.test.database import database_exists, database_delete, database_create
import os
import sys

dbname = 'dataspace_test_db'

if database_exists( dbname, bindir = os.environ['bindir']):
    status = database_delete( dbname, bindir = os.environ['bindir'])
    if status != 0:
        sys.exit( status)

status = database_create( dbname, bindir = os.environ['bindir'])
if status == 0:
    status = 77
sys.exit( status)

