import pexpect
import os
import sys

dbname = 'index_test_db'
attribute_1 = 'Person.firstName'
attribute_2 = 'Person.lastName'
attribute_3 = 'Person.age'
attribute_4 = 'Person.id'

# get the index statistics
command="%s/eyedbadmin index simulate --type=hash %s %s" % (os.environ['bindir'], dbname, attribute_1)
child = pexpect.spawn(command)
child.logfile = sys.stdout
r = child.expect("Index on %s" % (attribute_1,))
r = child.expect("Type: Hash")
r = child.expect("Key Count: 4096")
r = child.expect(pexpect.EOF)
child.close()
sys.exit(child.exitstatus)
