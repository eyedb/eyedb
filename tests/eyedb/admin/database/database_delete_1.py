from eyedb.test.command import run_simple_command
import os
import sys

dbname = 'new_database_test_db'

command="%s/eyedbadmin database delete %s" % (os.environ['bindir'], dbname)
status = run_simple_command( command, do_exit = False)
if status != 0:
    sys.exit( status)

copy_dbname = 'copy_database_test_db'
command="%s/eyedbadmin database delete %s" % (os.environ['bindir'], copy_dbname)
run_simple_command( command)
