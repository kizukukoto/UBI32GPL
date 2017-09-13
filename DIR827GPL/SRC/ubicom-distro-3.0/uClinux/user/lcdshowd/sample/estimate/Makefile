include ../../Makefile.inc
GUI_CFLAGS += -I../../include

all: image component touch

image: lib_image.o lib_label.o image.o
	$(CC) $(GUI_CFLAGS) lib_image.o lib_label.o image.o $(GUI_LDFLAGS) -o $@

component: component.o
	$(CC) $(GUI_CFLAGS) component.o $(GUI_LDFLAGS) -o $@

touch: touch.o
	$(CC) $(GUI_CFLAGS) touch.o $(GUI_LDFLAGS) -o $@

clean:
	@rm -rf *.o image component touch

%.o : %.c $(DEPENDENCY_HEADER)
	$(CC) $(GUI_CFLAGS) -c $*.c -o $*.o
