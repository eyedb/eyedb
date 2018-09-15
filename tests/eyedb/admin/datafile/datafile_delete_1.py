from eyedb.test.command import run_simple_command
import os

dbname = 'datafile_test_db'
datafile='/var/tmp/new_test.dat'

command="%s/eyedbadmin datafile delete %s %s" % (os.environ['bindir'], dbname,datafile)
run_simple_command( command)


