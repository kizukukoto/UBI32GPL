# http Makefile
# Copyright 2005, CAMEO Corporation
# All Rights Reserved.
#
KERNEL_PATH=$(ROOTDIR)/linux-2.6.x
APPS_PATH=$(ROOTDIR)/user
NVRAM_PATH=$(APPS_PATH)/nvram

ifeq ($(CONFIG_USER_OPENDNS),y)
CFLAGS += -DOPENDNS
endif


# add bridge ...


ifeq ($(CONFIG_PRODUCT),DIR827AX)
CFLAGS += -DCONFIG_BRIDGE_MODE
LDFLAGS += -lpthread 
endif

ifeq ($(CONFIG_PRODUCT),DIR857AX3)
CFLAGS += -DCONFIG_BRIDGE_MODE
LDFLAGS += -lpthread 
endif


# tr069 support openagent 2010.01.28
ifeq ($(CONFIG_USER_OPENAGENT),y)
CFLAGS += -DCONFIG_USER_OPENAGENT
endif


ifeq ($(CONFIG_USER_WL_ATH_5GHZ),y)
CFLAGS += -DUSER_WL_ATH_5GHZ
endif

ifeq ($(CONFIG_USER_HTTP_TIMEOUT),y)
CFLAGS += -DHTTP_TIMEOUT
endif

ifeq ($(CONFIG_USER_HTTPD_PURE),y)
CFLAGS += -DPURE_NETWORK_ENABLE
OBJS += pure.o pure_xml.o
endif

ifeq ($(CONFIG_USER_TC),y)
CFLAGS += -DCONFIG_USER_TC
endif

ifeq ($(CONFIG_USER_STREAMENGINE),y)
CFLAGS += -DCONFIG_USER_STREAMENGINE
endif

ifeq ($(CONFIG_USER_XML_AGENT),y)
CFLAGS += -DXML_AGENT
OBJS += widget.o md5.o
endif

ifeq ($(CONFIG_USER_AJAX),y)
CFLAGS += -DAJAX
OBJS += ajax.o
endif

ifeq ($(CONFIG_USER_WAN_8021X),y)
CFLAGS += -DCONFIG_USER_WAN_8021X
endif

# Support radvd or not
ifeq ($(CONFIG_USER_RADVD_RADVD),y)
CFLAGS += -DRADVD
endif

# Support 6rd or not
ifeq ($(CONFIG_USER_IPV6_6RD),y)
CFLAGS += -DIPV6_6RD 
endif

# Support graphic auth or not
ifeq ($(CONFIG_USER_AUTHGRAPH),y)
CFLAGS += -DAUTHGRAPH
OBJS += authgraph.o
endif

# Support HTTPS, Modified 20090409 by Yufa
ifeq ($(CONFIG_USER_HTTPS),y)
CFLAGS += -DHAVE_HTTPS -I$(ROOTDIR)/openssl/build/include
LDFLAGS += -L$(ROOTDIR)/openssl/build -lssl -lcrypto
endif

ifeq ($(CONFIG_HTTPD_USED_MUTIL_AUTH),y)
CFLAGS += -DHTTPD_USED_MUTIL_AUTH
endif

CFLAGS += -I. -I$(APPS_PATH)/sutil/ -I$(NVRAM_PATH) -I$(APPS_PATH)/udhcp -I$(APPS_PATH)/libplatform/ -I$(APPS_PATH)/header -I$(KERNEL_PATH)/include/
#CFLAGS += -D$(CONFIG_MODEL_NAME) -D$(PLATFORM)

ifeq ($(CONFIG_USER_LINUX_NVRAM),y)
CFLAGS += -I../artblock/
LDFLAGS += -L../artblock/ -lartblock
endif
ifeq ($(CONFIG_USER_IPV6_SUPPORT),y)
CFLAGS += -DIPV6_SUPPORT
LDFLAGS += -lipv6
endif

ifeq ($(CONFIG_USER_IPV6_PPPoE),y)
CFLAGS += -DIPV6_PPPoE
endif

ifeq ($(CONFIG_USER_STATELESS_WAN),y)
CFLAGS += -DIPV6_STATELESS_WAN
endif

# Check udhcpd reservation table setting by GUI or not
CFLAGS +=-DDHCPD_CHECK_RESERV_BY_GUI


#	Date:	2009-06-09
#	Name:	jimmy huang
#	Reason:	Avoid miniupnpd heavy loading cause not response some UPnP soap
#			We decrease the frequency for open nvram.conf
#			When http change nvram value, send signal to miniupnpd-1.3
#
ifeq ($(CONFIG_USER_UPNP_ATH_MINIUPNPD_1_3_CHECK_VS_NVRAM),y)
	CFLAGS += -DUPNP_CHECK_VS_NVRAM
endif

# Debud build
ifeq ($(CONFIG_USER_HTTPD_DEBUG),y)
CFLAGS += -DHTTPD_DEBUG
endif

ifeq ($(CONFIG_USER_HTTPD_DEBUG_PURE),y)
CFLAGS += -DHTTPD_DEBUG_PURE
endif

ifeq ($(CONFIG_USER_HTTPD_DEBUG_AJAX),y)
CFLAGS += -DHTTPD_DEBUG_AJAX
endif

ifeq ($(CONFIG_USER_HTTPD_DEBUG_HTTP_UTIL),y)
CFLAGS += -DHTTPD_DEBUG_HTTP_UTIL
endif

ifeq ($(CONFIG_USER_UNZIP_UNZIP),y)
CFLAGS +=  -DCONFIG_MODEL_NAME="\"$(CONFIG_PRODUCT)\""
CFLAGS += -I../tools/m17n/
OBJS += lp.o
endif

ifeq ($(CONFIG_USER_WEB_FILE_ACCESS),y)
OBJS += querys.o libaccess.o file_access.o
endif

# support login fail alert message
ifeq ($(CONFIG_USER_LOGIN_ALERT),y)
	CFLAGS +=-DGUI_LOGIN_ALERT
endif

#LDFLAGS += -L$(APPS_PATH)/nvram -lnvram -L$(APPS_PATH)/sutil -lsutil -lproject  -L$(APPS_PATH)/libplatform -lversion
LDFLAGS += -L$(NVRAM_PATH) -lnvram -L$(APPS_PATH)/sutil -lsutil -lproject -L$(APPS_PATH)/libplatform -lversion

LDFLAGS += -L$(APPS_PATH)/libplatform -lvct
LDFLAGS_CUST = -L$(APPS_PATH)/httpd -lhttpd
CFLAGS += -I$(ROOTDIR)/config 


OBJS += cgi.o ej.o httpd.o core.o cmobasicapi.o file.o log.o httpd_util.o customer.o chklst_version.o

ifeq ($(PLATFORM), MV88F6281)
OBJS += mime.o upgrade.o upload.o
endif
ifeq ($(PLATFORM), sl3516)
OBJS += firmware.o
endif

OBJ_CUSTOMER = customer.o

INSTALLDIR = $(TARGET)

all: httpd smac

lp.o: ./lp_version.h

do_not_clean:
	rm -f *.o *.a httpd smac *~
#	rm -f $(WWW_PATH)/*.o

install: all
	install -D httpd $(INSTALLDIR)/sbin/httpd
	$(STRIP) --strip-unneeded  $(INSTALLDIR)/sbin/httpd

uninstall:
	rm -rf $(INSTALLDIR)/sbin/httpd

romfs:
	$(ROMFSINST) httpd /sbin/httpd
	$(ROMFSINST) smac /sbin/smac

httpd:  $(OBJS)
	$(CC) -Wl,--defsym,__stacksize=0x80000 -o $@ $^ $(LDFLAGS)
#httpd: $(OBJS) $(OBJ_CUSTOMER)
#	$(AR) rcs libhttpd.a $(OBJS)
#	$(CC) $(OBJ_CUSTOMER) $(LDFLAGS_CUST) $(LDFLAGS) -o httpd

smac: smac.o
	$(CC) -o $@ $^ $(LDFLAGS)

httpd_gpl:  $(OBJ_CUSTOMER)
	$(CC) $(OBJ_CUSTOMER) $(LDFLAGS_CUST) $(LDFLAGS) -o httpd
