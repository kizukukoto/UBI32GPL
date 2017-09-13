OBJS = libdb.o libutil.o libnet.o libpartition.o liblang.o libhnap.o libhnap_xml.o

ifeq ($(CONFIG_MODEL_NAME), DIR-665_A1)
OBJS += libwlan.o libtime.o libupdatelog.o libdhcpd.o
endif
ifeq ($(CONFIG_VENDOR), Ubicom)
OBJS += libwlan.o libtime.o libupdatelog.o libdhcpd.o
endif


all: cgilib.a
	
cgilib.a: $(OBJS)
	$(AR) cr $@ $^
clean:
	@rm $(OBJS) cgilib.a -rf
