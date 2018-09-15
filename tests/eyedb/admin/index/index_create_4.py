import pexpect
import os
import sys

dbname = 'index_test_db'
attribute = 'Person.id'

# create the index
command="%s/eyedbadmin index create --type=hash %s %s" % (os.environ['bindir'], dbname, attribute)
child = pexpect.spawn(command)
child.logfile = sys.stdout
r = child.expect("Creating hash index on %s" % (attribute,))
r = child.expect(pexpect.EOF)
child.close()
sys.exit(child.exitstatus)
