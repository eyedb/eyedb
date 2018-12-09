TESTS_ENVIRONMENT= bindir=$(bindir) sbindir=$(sbindir) libdir=$(libdir) top_builddir=$(top_builddir) JAVA=$(JAVA) PYTHONPATH=$(top_srcdir)/tests/lib/python:$PYTHONPATH $(PYTHON) 

DIRECTORY_TESTS=\
test_store.py 

TESTS=\
$(DIRECTORY_TESTS) 

noinst_PYTHON= $(DIRECTORY_TESTS)
