SUBDIRS = share query store methods admin

all:
	@for dir in $(SUBDIRS); do $(MAKE) -C $${dir} $@; done

%:
	@for dir in $(SUBDIRS); do $(MAKE) -C $${dir} $@; done
