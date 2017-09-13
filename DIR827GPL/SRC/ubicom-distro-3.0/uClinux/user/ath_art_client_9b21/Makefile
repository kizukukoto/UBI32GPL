ART_SRC_ROOT ?= ./src/dk/mdk
EXEC = mdk_client.out

#CFLAGS += -save-temps

all:
	$(MAKE) -C $(ART_SRC_ROOT) PB42=1 -f makefile.soc.linux.mips all

romfs:
	$(MAKE) -C $(ART_SRC_ROOT)/client PB42=1 -f makefile.soc.linux.mips romfs
	$(ROMFSINST) /bin/$(EXEC)

clean:
	$(MAKE) -C $(ART_SRC_ROOT) PB42=1 -f makefile.soc.linux.mips clean
	rm -f $(EXEC)

.PHONY: all clean
