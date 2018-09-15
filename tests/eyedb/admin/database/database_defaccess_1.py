from eyedb.test.command import run_simple_command
import os

dbname = 'new_database_test_db'

command="%s/eyedbadmin database defaccess %s r" % (os.environ['bindir'], dbname)
run_simple_command( command)
