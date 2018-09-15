import os
from eyedb.test.command import run_simple_command

cpp_test = [ "%s/examples/C++Binding/generic/store/store" % (os.environ['top_builddir'],),
             'person_c',
             'raymond',
             '84']
run_simple_command( cpp_test)

