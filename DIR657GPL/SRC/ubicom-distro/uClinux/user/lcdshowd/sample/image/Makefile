include ../../Makefile.inc
GUI_CFLAGS += -I../../include

IMAGE_OBJS = image_action.o

all: image

image: $(IMAGE_OBJS)
	$(CC) $(GUI_CFLAGS) $(IMAGE_OBJS) $(GUI_LDFLAGS) -o $@

clean:
	@rm -rf *.o image

%.o : %.c $(DEPENDENCY_HEADER)
	$(CC) $(GUI_CFLAGS) -c $*.c -o $*.o
