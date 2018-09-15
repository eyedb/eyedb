from eyedb.test.command import run_simple_command
import os

dbname = 'datafile_test_db'
new_datafile='new_test.dat'
name='test'

command="%s/eyedbadmin datafile move --filedir=/var/tmp %s %s %s" % (os.environ['bindir'], dbname, name, new_datafile)
run_simple_command( command)



