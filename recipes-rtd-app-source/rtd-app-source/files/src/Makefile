CFLAGS = -I../inc/libcommon/ -lRuggedTelematics -lm -ldl -lpthread

all:   sample_application.o
	$(CC) $(CFLAGS) sample_application.o -o sample_application

sample_application.o:  sample_application.c
	$(CC) $(CFLAGS) -c sample_application.c

# To start over from scratch, type 'make clean'. This
# removes the executable file, as well as old .o object
# files and *~ backup files:
#
clean: 
	$(RM) *.o *~
