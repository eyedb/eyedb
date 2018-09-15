from eyedb.test.command import run_simple_command
import os

dbname = 'dataspace_test_db'
dspname='bar'
newdspname='bur'

command="%s/eyedbadmin dataspace rename %s %s %s" % (os.environ['bindir'], dbname, dspname, newdspname)
run_simple_command( command)
