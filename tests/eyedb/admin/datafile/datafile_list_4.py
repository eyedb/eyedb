import pexpect
import sys
import os

dbname = 'datafile_test_db'
datafile='new_test.dat'
name='new_test'

command="%s/eyedbadmin datafile list %s" % (os.environ['bindir'], dbname)
child = pexpect.spawn(command)
child.logfile = sys.stdout

child.expect( "Datafile #0")
child.expect( "Name *DEFAULT")
child.expect( "Dataspace #0 DEFAULT")
child.expect( "File *%s.dat" % (dbname,))
child.expect( "Maxsize")
child.expect( "Slotsize")
child.expect( "Oid Type *Logical")

child.expect( "Datafile #1")
child.expect( "Name *%s" % (name,))
child.expect( "File .*%s" % (datafile,))
child.expect( "Maxsize")
child.expect( "Slotsize")
child.expect( "Oid Type *Logical")

child.expect(pexpect.EOF)
child.close()
sys.exit(child.exitstatus)


