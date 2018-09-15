from eyedb.test.command import run_simple_command
import os

dbname = 'datafile_test_db'
name='test'
new_name='new_test'

command="%s/eyedbadmin datafile rename %s %s %s" % (os.environ['bindir'], dbname, name, new_name)
run_simple_command( command)
