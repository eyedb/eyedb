import sys
import os
from eyedb.test.server import server_stop

status = server_stop( sbindir = os.environ['sbindir'])
if status == 0:
    status = 77
sys.exit(status)

