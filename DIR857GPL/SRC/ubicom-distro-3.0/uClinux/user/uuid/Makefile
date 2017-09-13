OBJS += uuid.o sysdep.o md5.o
ALL = uuidgen

all: $(ALL)
clean:
	rm -f *.o *~ $(ALL)
install:
	cp -f $(ALL) $(TARGET)/sbin
$(ALL):$(OBJS)
	$(CC) -o $@ $^ 
romfs:
	$(ROMFSINST) uuidgen /bin

