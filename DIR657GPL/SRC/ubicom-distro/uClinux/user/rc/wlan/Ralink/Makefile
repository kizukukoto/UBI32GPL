CFLAGS += -I../../

OBJS = wlan.o do_config.o wlutils.o statistics.o mac_table.o channel.o eap.o wps.o wps_led.o

all: wlan.a
	@mv wlan.a ../../

wlan.a: $(OBJS)
	$(AR) cr $@ $^

clean:
	@rm -rf $(OBJS) wlan.a *~

%.o : %.c $(DEPENDENCY_HEADER)
	$(CC) -c $*.c -o $*.o $(CFLAGS)
