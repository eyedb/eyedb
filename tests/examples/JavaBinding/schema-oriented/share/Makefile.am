TESTS_ENVIRONMENT= bindir=$(bindir) sbindir=$(sbindir) top_srcdir=$(top_srcdir) top_builddir=$(top_builddir) PYTHONPATH=$(top_srcdir)/tests/lib/python:$PYTHONPATH $(PYTHON) 

DIRECTORY_TESTS=\
test_init.py 

TESTS=\
$(top_srcdir)/tests/lib/python/eyedb/test/test_start_server.py \
$(DIRECTORY_TESTS) \
$(top_srcdir)/tests/lib/python/eyedb/test/test_stop_server.py

noinst_PYTHON= $(DIRECTORY_TESTS)
