TESTS_ENVIRONMENT= bindir=$(bindir) sbindir=$(sbindir) libdir=$(libdir) top_builddir=$(top_builddir) PYTHONPATH=$(top_srcdir)/tests/lib/python:$PYTHONPATH $(PYTHON) 

DIRECTORY_TESTS=\
test_object_dump.py \
test_schema_dump.py 

TESTS=\
$(DIRECTORY_TESTS) 

noinst_PYTHON= $(DIRECTORY_TESTS)
