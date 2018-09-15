import pexpect
import sys
import os

dbname = 'dataspace_test_db'
dspname='DEFAULT'

command="%s/eyedbadmin dataspace getcurdat %s %s" % (os.environ['bindir'], dbname, dspname)
child = pexpect.spawn(command)
r = child.expect( "Datafile")
r = child.expect( "Name *DEFAULT")
r = child.expect( "Dataspace")
r = child.expect( "File *%s.dat" % (dbname,))
r = child.expect( "Maxsize")
child.expect( "Slotsize")
child.expect( "Oid Type *Logical")
r = child.expect(pexpect.EOF)
child.close()
sys.exit(child.exitstatus)


