import os
from eyedb.test.command import run_simple_command

# run the GettingStarted C++ part
cpp_test = "%s/examples/GettingStarted/persontest person_g" % (os.environ['top_builddir'],)
run_simple_command( cpp_test)

