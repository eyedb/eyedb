import pexpect
import sys

def test_simple_command( command, expected_output = None, expected_status = 0):
    child = pexpect.spawn(command)
    child.logfile = sys.stdout
    if expected_output is not None:
        for o in expected_output:
            child.expect(o)
    child.expect(pexpect.EOF)
    child.close()
    if child.exitstatus != expected_status:
        sys.exit( child.exitstatus)
    return child.exitstatus

def create_eyedb_database( dbname):
    # test if database exists
    dblist = "eyedbadmin2 database list %s" % (dbname,)
    child = pexpect.spawn( dblist)
    dblistmsg = "Database '%s' not found" % (dbname,)
    r = child.expect([dblistmsg, pexpect.EOF])
    # if it exists, delete it
    if r == 1:
        dbdelete = "eyedbadmin2 database delete %s" % (dbname,)
        (command_output, exitstatus) = pexpect.run (dbdelete, withexitstatus=1)
        if exitstatus != 0:
            sys.exit(exitstatus)
    # create the database
    dbcreate = "eyedbadmin2 database create %s" % (dbname,)
    (command_output, exitstatus) = pexpect.run (dbcreate, withexitstatus=1)
    if exitstatus != 0:
        sys.exit(exitstatus)
