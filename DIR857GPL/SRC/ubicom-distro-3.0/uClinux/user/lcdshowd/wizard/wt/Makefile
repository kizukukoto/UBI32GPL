CFLAGS += -Iinc

objs := libdhcpc.o dhcpc.o main.o pppoed.o

all:wt

wt: $(objs)
	$(CC) $(LDFLAGS) -o $@ $(objs)

clean:
	rm $(objs) wt -f
