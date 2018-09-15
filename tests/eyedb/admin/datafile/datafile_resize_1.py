from eyedb.test.command import run_simple_command
import os

dbname = 'datafile_test_db'
name='new_test'
new_size=4096

command="%s/eyedbadmin datafile resize %s %s %s" % (os.environ['bindir'], dbname, name, new_size)
run_simple_command( command)
