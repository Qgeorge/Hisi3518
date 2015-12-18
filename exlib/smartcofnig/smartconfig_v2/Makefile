CC = arm-hisiv100nptl-linux-gcc
CFLAGS = -g -pthread -o
smartlink: smartconfig.o smartlink.o 
	$(CC) $^ $(CFLAGS) $@

smartconfig.o: smartconfig.c smartconfig.h 
	$(CC) -c $<  
smartlink.o: smartlink.c smartconfig.h
	$(CC) -c $<
clean:
	$(RM) *.o smartlink

