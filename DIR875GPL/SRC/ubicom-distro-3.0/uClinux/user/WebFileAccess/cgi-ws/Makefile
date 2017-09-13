# Compiler command name

LIBS = -lmd5
INCLUDE = -Iinclude -I../md5
LIBPATH = -L../md5
EMBEDFILE = $(TARGET)/usr/bin
CGIFILE = $(pwd)/www/webfile_access
APPS_PATH=$(ROOTDIR)/user
#ifeq ($(CONFIG_USER_LINUX_NVRAM),y)
LIBS += -lnvram
INCLUDE += -I$(APPS_PATH)/nvram
LIBPATH += -L$(APPS_PATH)/nvram
#endif



# List all object files here
OBJS = main.o upload_file.o login.o wsinfo.o ../../libplatform/version.o

# Name of the executable
EXENAME = webfile_cgi.cgi

all:
	$(MAKE) ${EXENAME}
	
# Builds all object files and executable
${EXENAME} : $(OBJS)	
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(INCLUDE) $(LIBPATH) $(LIBS)

.c.o:	
	$(CC) $(CFLAGS) -c $< $(INCLUDE)

.PHONY: install
install:
	#mkdir $(INSTALLDIR)/var/tmp/storage
	$(STRIP) $(EXENAME)
	#cp $(EXENAME) $(CGIFILE)/$(EXENAME)
	#ln -sf $(EMBEDFILE)/$(EXENAME) $(CGIFILE)/$(EXENAME) 
	$(INSTALL)	$(EXENAME) $(EMBEDFILE)
#777	
# Clean up
do_not_clean:
	rm -f $(OBJS)  $(EXENAME)
	rm -f $(EMBEDFILE)/$(EXENAME)	
	
#romfs:
#	$(ROMFSINST) webfile_cgi.cgi /www/webfile_access/webfile_cgi.cgi
