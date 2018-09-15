import pexpect
import sys
from eyedb.test.command import run_simple_command

def database_exists( dbname, bindir = None):
    dblist_cmd = ''
    if bindir is not None:
        dblist_cmd = bindir + '/'
    dblist_cmd = dblist_cmd + 'eyedbadmin database list --dbname ' + dbname
    child = pexpect.spawn( dblist_cmd)
    r = child.expect([dbname, "Database '%s' not found" % (dbname,)])
    return r == 0

def database_delete( dbname, bindir = None):
    dbdelete_cmd = ''
    if bindir is not None:
        dbdelete_cmd = bindir + '/'
    dbdelete_cmd = dbdelete_cmd + 'eyedbadmin database delete ' + dbname
    return run_simple_command( dbdelete_cmd, do_exit=False)

def database_create( dbname, force_create = False, bindir = None):
    if force_create and database_exists( dbname, bindir):
        status = database_delete( dbname, bindir)
        if status != 0:
            return status
    dbcreate_cmd = ''
    if bindir is not None:
        dbcreate_cmd = bindir + '/'
    dbcreate_cmd = dbcreate_cmd + 'eyedbadmin database create ' + dbname
    return run_simple_command( dbcreate_cmd, do_exit=False)
