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
$(DIRECTORY_TESTS) 

noinst_PYTHON= $(DIRECTORY_TESTS)

EXTRA_DIST= person.odl
