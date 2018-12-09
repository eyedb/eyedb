TESTS_ENVIRONMENT= bindir=$(bindir) sbindir=$(sbindir) PYTHONPATH=$(top_srcdir)/tests/lib/python:$$PYTHONPATH $(PYTHON) 

DIRECTORY_TESTS=\
database_create_1.py \
database_list_1.py \
database_move_1.py \
database_list_2.py \
database_rename_1.py \
database_list_3.py \
database_copy_1.py \
database_list_4.py \
database_defaccess_1.py \
database_list_5.py \
database_delete_1.py \
database_list_6.py 

TESTS=\
$(DIRECTORY_TESTS) 

noinst_PYTHON= $(DIRECTORY_TESTS)
