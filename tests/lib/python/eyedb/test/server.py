from eyedb.test.command import run_simple_command

def server_status( sbindir = None):
    status_cmd = ''
    if sbindir is not None:
        status_cmd = sbindir + '/'
    status_cmd = status_cmd + 'eyedbctl status'
    return run_simple_command( status_cmd, do_exit=False)

def server_start( sbindir = None):
    if  server_status( sbindir) != 0:
        start_cmd = ''
        if sbindir is not None:
            start_cmd = sbindir + '/'
        start_cmd = start_cmd + 'eyedbctl start'
        status = run_simple_command( start_cmd, do_exit=False)
        return status
    return 0

def server_stop( sbindir = None):
    if  server_status( sbindir) == 0:
        stop_cmd = ''
        if sbindir is not None:
            stop_cmd = sbindir + '/'
        stop_cmd = stop_cmd + 'eyedbctl stop'
        status = run_simple_command( stop_cmd, do_exit=False)
        return status
    return 0
