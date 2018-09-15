import pexpect
import sys
import os

username='toto'
password='titi'
newpassword='tata'

command="%s/eyedbadmin user passwd %s " % ( os.environ['bindir'], username)
child = pexpect.spawn(command)
child.logfile = sys.stdout
r = child.expect("user old password: ")
child.sendline( password)
r = child.expect("user new password: ")
child.sendline( newpassword)
r = child.expect("retype user new password: ")
child.sendline( newpassword)
r = child.expect(pexpect.EOF)
child.close()
sys.exit(child.exitstatus)
