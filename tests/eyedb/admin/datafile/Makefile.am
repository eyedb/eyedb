TESTS_ENVIRONMENT= bindir=$(bindir) sbindir=$(sbindir) PYTHONPATH=$(top_srcdir)/tests/lib/python:$$PYTHONPATH $(PYTHON) 

DIRECTORY_TESTS=\
datafile_create_database_1.py \
datafile_list_1.py \
datafile_create_1.py \
datafile_list_2.py \
datafile_move_1.py \
datafile_list_3.py \
datafile_rename_1.py \
datafile_list_4.py \
datafile_resize_1.py \
datafile_list_5.py \
datafile_delete_1.py \
datafile_list_6.py \
datafile_delete_database_1.py 

TESTS=\
$(top_srcdir)/tests/lib/python/eyedb/test/test_start_server.py \
$(DIRECTORY_TESTS) \
$(top_srcdir)/tests/lib/python/eyedb/test/test_stop_server.py

noinst_PYTHON= $(DIRECTORY_TESTS)
