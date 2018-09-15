import pexpect
import sys
import os

dbname = 'database_test_db'
filedir = '/var/tmp'

command="%s/eyedbadmin database list %s" % (os.environ['bindir'], dbname)
child = pexpect.spawn(command)
child.logfile = sys.stdout
child.expect( "Database Name")
child.expect( " *%s" % (dbname,))
child.expect( "Database File")
child.expect( " *%s/%s\.dbs" % (filedir,dbname,))
child.expect(pexpect.EOF)
child.close()
sys.exit(child.exitstatus)


