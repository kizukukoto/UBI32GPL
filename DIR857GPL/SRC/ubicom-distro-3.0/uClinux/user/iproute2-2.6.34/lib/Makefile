CFLAGS += -fPIC

__CFLAGS__ = $(CFLAG)

CFLAGS = -I../include $(__CFLAGS__)

UTILOBJ=utils.o rt_names.o ll_types.o ll_proto.o ll_addr.o inet_proto.o

NLOBJ=ll_map.o libnetlink.o

all: libnetlink.a libutil.a

libnetlink.a: $(NLOBJ)
	$(AR) rcs $@ $(NLOBJ)

libutil.a: $(UTILOBJ) $(ADDLIB)
	$(AR) rcs $@ $(UTILOBJ) $(ADDLIB)
romfs:

install:

clean:
	rm -f $(NLOBJ) $(UTILOBJ) $(ADDLIB) libnetlink.a libutil.a

