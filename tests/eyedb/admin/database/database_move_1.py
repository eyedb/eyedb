from eyedb.test.command import run_simple_command
import os

dbname = 'database_test_db'
filedir = '/var/tmp'

command="%s/eyedbadmin database move --filedir=%s %s" % (os.environ['bindir'], filedir, dbname,)
run_simple_command( command)
