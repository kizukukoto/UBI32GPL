include ../Makefile.inc

MODULE_LIBS = ../src/liblcdrouter.so

all: $(PROJECTNAME)

$(PROJECTNAME):
	$(CC) -I../include $(GUI_CFLAGS) -o $(PROJECTNAME) $(PROJECTNAME)_src/$(PROJECTNAME).c $(MODULE_LIBS) $(GUI_LDFLAGS) 

install:
	test -z "../build/bin" || mkdir -p ../build/bin
	cp $(PROJECTNAME) ../build/bin

clean:
	test -z "$(PROJECTNAME)" || rm -f $(PROJECTNAME)
