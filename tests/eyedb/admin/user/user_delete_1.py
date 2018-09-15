from eyedb.test.command import run_simple_command
import os

username='toto'

command="%s/eyedbadmin user delete %s" % (os.environ['bindir'], username)
run_simple_command( command)

