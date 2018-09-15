from eyedb.test.command import run_simple_command
import sys
import os

dbname = 'dataspace_test_db'
dspname='bar'
datname='joe.dat'

# create the data file
command = "%s/eyedbadmin datafile create %s %s" % (os.environ['bindir'], dbname, datname)
status = run_simple_command( command, do_exit = False)
if status != 0:
    sys.exit( status)

# update the dataspace
command="%s/eyedbadmin dataspace update %s %s %s" % (os.environ['bindir'], dbname, dspname, datname)
run_simple_command( command)
