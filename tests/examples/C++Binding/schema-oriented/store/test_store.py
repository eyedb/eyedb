import os
from eyedb.test.command import run_simple_command

cpp_test = [ "%s/examples/C++Binding/schema-oriented/store/store" % (os.environ['top_builddir'],),
             'person_c',
             'nadine',
             '32',
             'henry']
run_simple_command( cpp_test)

