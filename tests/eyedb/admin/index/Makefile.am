TESTS_ENVIRONMENT= bindir=$(bindir) sbindir=$(sbindir) PYTHONPATH=$(top_srcdir)/tests/lib/python:$PYTHONPATH $(PYTHON) 

DIRECTORY_TESTS=\
index_create_database_1.py \
index_create_1.py \
index_create_2.py \
index_create_3.py \
index_create_4.py \
index_list_1.py \
index_list_2.py \
index_stats_1.py \
index_stats_2.py \
index_simulate_1.py \
index_delete_1.py \
index_list_3.py \
index_delete_database_1.py 

TESTS=\
$(top_srcdir)/tests/lib/python/eyedb/test/test_start_server.py \
$(DIRECTORY_TESTS) \
$(top_srcdir)/tests/lib/python/eyedb/test/test_stop_server.py 

noinst_PYTHON= $(DIRECTORY_TESTS)

EXTRA_DIST= person.odl
