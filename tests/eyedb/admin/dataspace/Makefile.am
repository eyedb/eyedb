TESTS_ENVIRONMENT= bindir=$(bindir) sbindir=$(sbindir) PYTHONPATH=$(top_srcdir)/tests/lib/python:$PYTHONPATH $(PYTHON) 

DIRECTORY_TESTS=\
dataspace_create_database_1.py \
dataspace_list_1.py \
dataspace_list_2.py \
dataspace_list_3.py \
dataspace_create_1.py \
dataspace_update_1.py \
dataspace_rename_1.py \
dataspace_setdef_1.py \
dataspace_getdef_1.py \
dataspace_setdef_2.py \
dataspace_getcurdat_1.py \
dataspace_delete_1.py \
dataspace_delete_database_1.py 

TESTS=\
$(top_srcdir)/tests/lib/python/eyedb/test/test_start_server.py \
$(DIRECTORY_TESTS) \
$(top_srcdir)/tests/lib/python/eyedb/test/test_stop_server.py

noinst_PYTHON= $(DIRECTORY_TESTS)
