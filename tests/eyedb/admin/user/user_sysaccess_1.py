from eyedb.test.command import run_simple_command
import sys
import os

username='toto'

command="%s/eyedbadmin user sysaccess %s dbcreate" % ( os.environ['bindir'], username)
status = run_simple_command( command, do_exit = False)
if status != 0:
    sys.exit( status)

command="%s/eyedbadmin user sysaccess %s setuserpasswd" % ( os.environ['bindir'], username)
run_simple_command( command)


