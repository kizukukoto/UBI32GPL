CFLAGS += -I../../

OBJS = wlan.o

all: wlan.a
	@mv wlan.a ../../

wlan.a: $(OBJS)
	$(AR) cr $@ $^

clean:
	@rm -rf $(OBJS) wlan.a *~

%.o : %.c $(DEPENDENCY_HEADER)
	$(CC) -c $*.c -o $*.o $(CFLAGS)
