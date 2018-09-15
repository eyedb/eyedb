import pexpect
import sys
import os

username='toto'
command="%s/eyedbadmin user list" % (os.environ['bindir'],)
child = pexpect.spawn( command)
child.logfile = sys.stdout
child.expect("name .*: \"%s\"\r\n" % (username,))
child.expect("sysaccess .*: SET_USER_PASSWD_SYSACCESS_MODE\r\n")
child.expect(pexpect.EOF)
child.close()
sys.exit(child.exitstatus)


