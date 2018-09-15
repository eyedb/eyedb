import pexpect
import sys
import os

command="%s/eyedbadmin user list" % (os.environ['bindir'],)
child = pexpect.spawn( command)
child.logfile = sys.stdout
child.expect("name .*: .* \[strict unix user\]\r\n")
child.expect("sysaccess .*: SUPERUSER_SYSACCESS_MODE\r\n")
child.expect(pexpect.EOF)
child.close()
sys.exit(child.exitstatus)


