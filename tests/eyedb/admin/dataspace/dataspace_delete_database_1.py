from eyedb.test.database import database_exists, database_delete, database_create
import os
import sys

dbname = 'dataspace_test_db'

status = database_delete( dbname, bindir = os.environ['bindir'])
if status == 0:
    status = 77
sys.exit( status)
