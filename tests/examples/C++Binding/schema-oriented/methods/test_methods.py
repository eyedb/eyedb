import os
from eyedb.test.command import run_simple_command

cpp_test = [ "%s/examples/C++Binding/schema-oriented/methods/methods" % (os.environ['top_builddir'],),
             'person_c',
             'john',
             '12']
run_simple_command( cpp_test)

