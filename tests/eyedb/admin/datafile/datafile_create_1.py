from eyedb.test.command import run_simple_command
import os

dbname = 'datafile_test_db'
datafile='test.dat'
name='test'

command="%s/eyedbadmin datafile create --name=%s %s %s" % (os.environ['bindir'], name, dbname, datafile)
run_simple_command( command)
