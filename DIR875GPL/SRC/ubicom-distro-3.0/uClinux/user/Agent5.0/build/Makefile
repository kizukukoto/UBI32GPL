.PHONY: all clean


# 2010.07.08 direct tool chain 
CC_TEST=0

ifeq ($(CC),cc)
CC = ubicom32-linux-uclibc-gcc -march=ubicom32v5 -DIP8000
ROOTDIR= /home/ubicom/ubicom/ubicom-distro_SDK_2.0/uClinux
CC_TEST=1
endif






SRCPATH = ../source
HDRPATH = ../include
INFPATH = ../interface
XMLPATH = $(INFPATH)/xml
SQLPATH = $(INFPATH)/sqlite
WARPATH = ../war
LINUXPATH = $(WARPATH)/linux
LINUXINCLUDE = $(LINUXPATH)/include

BUILDPATH = ../build
OBJPATH = $(BUILDPATH)/obj
BINPATH = $(BUILDPATH)/bin

#CC = gcc
CFLAGS = -O -Wall -Werror
#DEFS = -DCODE_DEBUG -DVT100_COMPAT -DUSE_C99 -D__ENABLE_SSL__ -DWITH_STRPTIME
#DEFS = -DVT100_COMPAT -DUSE_C99 -DCODE_DEBUG -D__ENABLE_SSL__ -DTR069_WIB -DTR143 -DTR111 -DTR196 -D__V4_2
#DEFS = -DVT100_COMPAT -DUSE_C99 -DCODE_DEBUG -D__ENABLE_SSL__ -DTR069_WIB -DTR143 -DTR111 -DTR196
DEFS = -DVT100_COMPAT -DUSE_C99 -DCODE_DEBUG -D__ENABLE_SSL__ 
#LIBS = -lssl -lpthread -lsqlite3
LIBS = -lssl -lpthread

# try openall
KERNEL_PATH=$(ROOTDIR)/linux-2.6.x
APPS_PATH=$(ROOTDIR)/user
NVRAM_PATH=$(APPS_PATH)/nvram

LIBS += -L$(ROOTDIR)/lib
LIBS += -lcrypto

INCLUDES = -I$(HDRPATH) -I$(LINUXINCLUDE) -I$(XMLPATH)
#INCLUDES = -I$(HDRPATH) -I$(LINUXINCLUDE) -I$(SQLPATH)

# try add openssl
INCLUDES += -I$(ROOTDIR)/include

INCLUDES += -I$(NVRAM_PATH)
INCLUDES += -I$(APPS_PATH)/libplatform/



VPATH = $(SRCPATH):$(XMLPATH):$(LINUXPATH)/string:$(LINUXPATH)/math:$(LINUXPATH)/file:$(LINUXPATH)/socket:$(LINUXPATH)/time:$(LINUXPATH)/error
#VPATH = $(SRCPATH):$(SQLPATH):$(LINUXPATH)/string:$(LINUXPATH)/math:$(LINUXPATH)/file:$(LINUXPATH)/socket:$(LINUXPATH)/time:$(LINUXPATH)/error

SRCS=$(wildcard $(SRCPATH)/*.c) $(wildcard $(XMLPATH)/*.c)  $(wildcard $(LINUXPATH)/*/*.c)
#SRCS=$(wildcard $(SRCPATH)/*.c) $(wildcard $(SQLPATH)/*.c) $(wildcard $(LINUXPATH)/*/*.c)


#try add some src
#APP_SRCS += $(APPS_PATH)/libplatform/linux_vct.c

HEADERS=$(wildcard $(HDRPATH)/*.h)
NOTDIRSRCS=$(notdir $(SRCS))

#NOTDIRSRCS += $(APPS_PATH)/libplatform/linux_vct.c
#NOTDIRSRCS += $(APPS_PATH)/libplatform/version.c


OBJS = $(patsubst %.c,$(OBJPATH)/%.o,$(NOTDIRSRCS))

# try
LIBS += -L$(APPS_PATH)/libplatform -lversion
LIBS += -L$(APPS_PATH)/libplatform -lvct
LIBS += -L$(NVRAM_PATH) -lnvram
LIBS += -L$(APPS_PATH)/sutil -lsutil
LIBS += -lproject

INCLUDES += -I$(APPS_PATH)/sutil/

# direct use .o (httppd_util)
LIBS += $(APPS_PATH)/httpd/httpd_util.o
LIBS += -lipv6

all: tr

tr: $(OBJS)
	echo $(ROOTDIR)
	echo $(LIBS)

# try add
#	$(CC) $(CFLAGS) -c -o $(OBJPATH)/version.o  $(APPS_PATH)/libplatform/version.c   $(INCLUDES) $(DEFS)
#	$(CC) $(CFLAGS) -c -o $(OBJPATH)/httpd_util.o  $(APPS_PATH)/httpd/httpd_util.c   $(INCLUDES) $(DEFS)

	
	@mkdir -p $(BINPATH)
	$(CC) -s -o $(BINPATH)/$@ $^ $(LIBS)
	@echo Compile OneAgent-TR successful....

# 2010.07.08 direct tool chain 
# copy to tftp for...
ifeq ($(CC_TEST),1)

	cp $(BINPATH)/$@ /home/ubicom/tftp/tr069
	cp ../conf/tr.xml /home/ubicom/tftp
	cp ../conf/update /home/ubicom/tftp
	cp ../conf/tr_652EU.xml /home/ubicom/tftp/tr.xml
endif

$(OBJS): $(OBJPATH)/%.o: %.c
	@mkdir -p $(OBJPATH)
	$(CC) $(CFLAGS) -c -o $@  $< $(INCLUDES) $(DEFS)

clean:
	rm -rf $(BINPATH)/* $(OBJPATH)/* 
