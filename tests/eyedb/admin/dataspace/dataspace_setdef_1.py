from eyedb.test.command import run_simple_command
import os

dbname = 'dataspace_test_db'
dspname='bur'

command="%s/eyedbadmin dataspace setdef %s %s" % (os.environ['bindir'], dbname,dspname)
run_simple_command( command)
