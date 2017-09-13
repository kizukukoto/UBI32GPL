VER = hostapd-0.5.11

#for debugging purposes
CFLAGS += -O0

all: $(VER)/Makefile
	export LIBS="$(LDLIBS)"; \
	$(MAKE) CC="$(CC) $(CFLAGS)" LDFLAGS="$(LDFLAGS)" -C $(VER)

clean:
	$(MAKE) -C $(VER) clean

romfs:
	$(ROMFSINST) $(VER)/hostapd /bin/hostapd
	$(ROMFSINST) $(VER)/hostapd_cli /bin/hostapd_cli

.PHONY: all clean romfs

