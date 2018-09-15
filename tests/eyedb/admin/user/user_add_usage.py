import pexpect
import sys
import os

command="%s/eyedbadmin user add" % (os.environ['bindir'],)
child = pexpect.spawn( command)
child.logfile = sys.stdout
child.expect('eyedbadmin user add \[--help\] \[--unix\] \[--strict-unix\] \[USER \[PASSWORD\]]')
child.expect('  --help        Displays the current help')
child.expect('  --unix        ')
child.expect('  --strict-unix ')
child.expect('  USER          User name')
child.expect('  PASSWORD      Password for specified user')
child.close()
if child.exitstatus == 1:
    sys.exit(0)
sys.exit( child.exitstatus)
