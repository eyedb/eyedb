import pexpect
import sys
import os

dbname = 'dataspace_test_db'

command="%s/eyedbadmin dataspace getdef %s" % (os.environ['bindir'], dbname)
child = pexpect.spawn(command)
r = child.expect( "Dataspace #1")
r = child.expect( "Name bur")
r = child.expect( "Datafile")
r = child.expect( "File")
r = child.expect( "Maxsize")
child.expect( "Slotsize")
child.expect( "Oid Type Logical")
r = child.expect(pexpect.EOF)
child.close()
sys.exit(child.exitstatus)


