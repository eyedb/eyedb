from eyedb.test.command import run_simple_command
import os

dbname = 'database_test_db'
new_dbname = 'new_database_test_db'

command="%s/eyedbadmin database rename %s %s" % (os.environ['bindir'], dbname, new_dbname)
run_simple_command( command)
