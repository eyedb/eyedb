TESTS_ENVIRONMENT= bindir=$(bindir) sbindir=$(sbindir) libdir=$(libdir) top_srcdir=$(top_srcdir) top_builddir=$(top_builddir) JAVA=$(JAVA) PYTHONPATH=$(top_srcdir)/tests/lib/python:$PYTHONPATH $(PYTHON) 

DIRECTORY_TESTS=\
test_init.py \
test_cpp.py \
test_java.py 

TESTS=\
$(top_srcdir)/tests/lib/python/eyedb/test/test_start_server.py \
$(DIRECTORY_TESTS) \
$(top_srcdir)/tests/lib/python/eyedb/test/test_stop_server.py

noinst_PYTHON= $(DIRECTORY_TESTS)
