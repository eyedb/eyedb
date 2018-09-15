SUBDIRS = basic query store schema

all:
	@for dir in $(SUBDIRS); do $(MAKE) -C $${dir} $@; done

init:
	@echo $@

%:
	@for dir in $(SUBDIRS); do $(MAKE) -C $${dir} $@; done
