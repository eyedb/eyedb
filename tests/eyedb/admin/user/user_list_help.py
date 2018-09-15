import pexpect
import sys
import os

command='%s/eyedbadmin user list --help' % (os.environ['bindir'],)

child = pexpect.spawn( command)
child.logfile = sys.stdout
r = child.expect('eyedbadmin user list \[--help\] \[USER\]')
r = child.expect('\n')
r = child.expect('  --help Displays the current help')
r = child.expect('  USER   User name')
r = child.expect(pexpect.EOF)
child.close()
if child.exitstatus == 1:
    sys.exit(0)
sys.exit( child.exitstatus)



