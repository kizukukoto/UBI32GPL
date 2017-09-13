.EXPORT_ALL_VARIABLES:

VER = lcdrouter-1.0

.PHONY: clean romfs all

all: 
	$(MAKE) -C $(VER)
	$(MAKE) -C $(VER) install

romfs:
	$(ROMFSINST) -d $(ROOTDIR)/user/lcdrouter/$(VER)/build/bin /bin
	$(ROMFSINST) -d $(ROOTDIR)/user/lcdrouter/$(VER)/build/lib /usr/lib
	$(ROMFSINST) -d $(ROOTDIR)/user/lcdrouter/$(VER)/data/theme /share/LiTE/lcdrouter/theme

clean:
	$(MAKE) -C $(VER) clean

