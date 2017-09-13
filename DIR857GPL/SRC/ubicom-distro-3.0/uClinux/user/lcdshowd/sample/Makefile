SUBDIRS = sliding window image estimate leak

all:
	@for d in $(SUBDIRS); do $(MAKE) -C $$d all || exit 1 ; done

clean:
	@for d in $(SUBDIRS); do $(MAKE) -C $$d clean; done

PHONY: all clean
