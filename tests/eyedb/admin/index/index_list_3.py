import pexpect
import os
import sys

dbname = 'index_test_db'
attribute_1 = 'Person.firstName'
attribute_2 = 'Person.lastName'
attribute_3 = 'Person.age'
attribute_4 = 'Person.id'

# list the index
command="%s/eyedbadmin index list %s" % (os.environ['bindir'], dbname)
child = pexpect.spawn(command)
child.logfile = sys.stdout
r = child.expect("hash index on %s" % (attribute_1,))
r = child.expect("btree index on %s" % (attribute_3,))
r = child.expect("hash index on %s" % (attribute_4,))
r = child.expect(pexpect.EOF)
child.close()
sys.exit(child.exitstatus)
