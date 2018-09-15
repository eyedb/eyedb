from eyedb.test.command import run_simple_command
import os

dbname = 'new_database_test_db'
copy_dbname = 'copy_database_test_db'

command="%s/eyedbadmin database copy %s %s" % (os.environ['bindir'], dbname, copy_dbname)
run_simple_command( command)
