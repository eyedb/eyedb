import pexpect
import os
import sys

dbname = 'index_test_db'
attribute = 'Person.lastName'

# delete the index
command="%s/eyedbadmin index delete %s %s" % (os.environ['bindir'], dbname, attribute)
child = pexpect.spawn(command)
child.logfile = sys.stdout
r = child.expect("Deleting index %s" % (attribute,))
r = child.expect(pexpect.EOF)
child.close()
sys.exit(child.exitstatus)
