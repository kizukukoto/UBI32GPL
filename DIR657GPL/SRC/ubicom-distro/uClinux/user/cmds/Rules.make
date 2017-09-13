all:built-in.o
	
built-in.o: $(OBJS)
	$(LD) -r -o $@ $(OBJS)
clean:
	@rm $(OBJS) built-in.o -rf
