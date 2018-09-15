import pexpect
import sys
import os

command="%s/eyedbadmin user dbaccess" % (os.environ['bindir'],)
child = pexpect.spawn( command)
child.logfile = sys.stdout
child.expect("eyedbadmin user dbaccess \[--help\] USER DBNAME MODE")
child.close()
if child.exitstatus == 1:
    sys.exit(0)
sys.exit( child.exitstatus)
