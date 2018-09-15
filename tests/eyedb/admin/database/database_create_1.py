from eyedb.test.command import run_simple_command
import os

dbname = 'database_test_db'

command="%s/eyedbadmin database create %s" % (os.environ['bindir'], dbname,)
run_simple_command( command)
