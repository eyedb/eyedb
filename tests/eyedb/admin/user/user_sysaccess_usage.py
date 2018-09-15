import pexpect
import sys
import os

command="%s/eyedbadmin user sysaccess" % (os.environ['bindir'],)

child = pexpect.spawn( command)
child.logfile = sys.stdout
r = child.expect("eyedbadmin user sysaccess \[--help\] USER \['\+' combination of\] dbcreate\|adduser\|deleteuser\|setuserpasswd\|admin\|superuser\|no")
r = child.expect( pexpect.EOF)
child.close()
if child.exitstatus == 1:
    sys.exit(0)
sys.exit( child.exitstatus)
