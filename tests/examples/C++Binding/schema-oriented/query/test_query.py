import os
from eyedb.test.command import run_simple_command

cpp_test = [ "%s/examples/C++Binding/schema-oriented/query/query" % (os.environ['top_builddir'],),
             'person_c',
             'select Person.name ~ "j"']
run_simple_command( cpp_test)

