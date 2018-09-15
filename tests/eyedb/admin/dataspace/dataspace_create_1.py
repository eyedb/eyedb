from eyedb.test.command import run_simple_command
import os
import sys

dbname = 'dataspace_test_db'
dspname='bar'
datname='bar.dat'

# create the data file
command = "%s/eyedbadmin datafile create %s %s" % (os.environ['bindir'], dbname, datname)
status = run_simple_command( command, do_exit = False)
if status != 0:
    sys.exit( status)

# create the dataspace
command="%s/eyedbadmin dataspace create %s %s %s" % (os.environ['bindir'], dbname, dspname, datname)
run_simple_command( command)
