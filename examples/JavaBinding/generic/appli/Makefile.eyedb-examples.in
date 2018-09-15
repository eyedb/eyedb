SUBDIRS = basic collections query store schema

all:
	@for dir in $(SUBDIRS); do $(MAKE) -C $${dir} $@; done

%:
	@for dir in $(SUBDIRS); do $(MAKE) -C $${dir} $@; done

