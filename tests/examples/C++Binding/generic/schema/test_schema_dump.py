import os
from eyedb.test.command import run_simple_command

cpp_test = [ "%s/examples/C++Binding/generic/schema/schema_dump" % (os.environ['top_builddir'],),
             'person_c']
run_simple_command( cpp_test)

