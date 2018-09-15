import pexpect
import sys
import os

command="%s/eyedbadmin user delete" % (os.environ['bindir'],)

child = pexpect.spawn( command)
child.logfile = sys.stdout
r = child.expect('eyedbadmin user delete \[--help\] USER')
r = child.expect( pexpect.EOF)
child.close()
if child.exitstatus == 1:
    sys.exit(0)
sys.exit( child.exitstatus)
