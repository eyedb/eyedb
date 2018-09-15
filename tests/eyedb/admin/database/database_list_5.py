import pexpect
import sys
import os

dbname = 'new_database_test_db'

command="%s/eyedbadmin database list %s" % (os.environ['bindir'], dbname)
child = pexpect.spawn(command)
child.logfile = sys.stdout
child.expect( "Default Access")
child.expect( " *READ_DBACCESS_MODE")
child.expect(pexpect.EOF)
child.close()
sys.exit(child.exitstatus)

