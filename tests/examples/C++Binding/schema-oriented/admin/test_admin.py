import os
import sys
from eyedb.test.command import run_simple_command

cpp_test = "%s/examples/C++Binding/schema-oriented/admin/admin" % (os.environ['top_builddir'],)
status = run_simple_command( cpp_test, do_exit=False)
print
sys.exit(status)

