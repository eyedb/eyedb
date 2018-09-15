import os
from eyedb.test.command import run_simple_command

cpp_test = [ "%s/examples/C++Binding/generic/query/query" % (os.environ['top_builddir'],),
             'person_c',
             'select Person.age']
run_simple_command( cpp_test)

