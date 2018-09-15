import pexpect
import sys
import os

dbname = 'dataspace_test_db'
dspname='XYZZY'

command="%s/eyedbadmin dataspace list %s %s " % (os.environ['bindir'], dbname, dspname)
child = pexpect.spawn(command)
child.expect( "eyedb error: dataspace %s not found in database %s" % (dspname, dbname))
r = child.expect(pexpect.EOF)
child.close()
if child.exitstatus == 1:
    sys.exit(0)
sys.exit(child.exitstatus)


