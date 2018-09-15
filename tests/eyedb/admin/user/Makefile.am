TESTS_ENVIRONMENT= bindir=$(bindir) sbindir=$(sbindir) PYTHONPATH=$(top_srcdir)/tests/lib/python:$PYTHONPATH $(PYTHON) 

DIRECTORY_TESTS=\
user_list_help.py \
user_list_1.py \
user_list_not_found.py \
user_add_usage.py \
user_add_1.py \
user_list_2.py \
user_sysaccess_usage.py \
user_sysaccess_1.py \
user_list_3.py \
user_dbaccess_usage.py \
user_passwd_1.py \
user_delete_usage.py \
user_delete_1.py 

TESTS=\
$(top_srcdir)/tests/lib/python/eyedb/test/test_start_server.py \
$(DIRECTORY_TESTS) \
$(top_srcdir)/tests/lib/python/eyedb/test/test_stop_server.py

noinst_PYTHON= $(DIRECTORY_TESTS)
