import pexpect
import sys
import os

username='toto'
password='titi'

command="%s/eyedbadmin user add %s" % (os.environ['bindir'], username)
child = pexpect.spawn(command)
child.logfile = sys.stdout
r = child.expect("%s password: " % (username,))
child.sendline( password)
r = child.expect("retype %s password: " % (username,))
child.sendline( password)
r = child.expect(pexpect.EOF)
child.close()
sys.exit(child.exitstatus)


