# Compiler command name
LIBSNAME = libmd5
#EMBEDLIB = $(FS_PATH)/lib

INCLUDE = -I.
OBJ = md5.o

md5.o: md5.c 
	$(CC) $(CFLAGS) -fPIC -o $@ -c $< $(INCLUDE)
	$(CC) -shared -o $(LIBSNAME).so $(OBJ)
		
#.PHONY: install
#install:
#		cp -f $(LIBSNAME).so $(EMBEDLIB)

romfs:
	$(ROMFSINST) $(LIBSNAME).so /lib


.PHONY: clean md5.c
clean:
	rm -rf *.o $(LIBSNAME).so*
	rm -f $(ROMFSDIR)/lib/$(LIBSNAME).so*
