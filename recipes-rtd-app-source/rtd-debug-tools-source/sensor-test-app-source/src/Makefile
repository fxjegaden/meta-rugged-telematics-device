CFLAGS = -I../inc/libcommon/ -lRuggedTelematics -lm -ldl -g -Wall -fPIC -lpthread

all: sensor_test_application.o
	$(CC) $(CFLAGS) sensor_test_application.o -o sensor_test

sensor_test_application.o: sensor_test_application.c
	$(CC) $(CFLAGS) -c sensor_test_application.c

# To start over from scratch, type 'make clean'. This
# removes the executable file, as well as old .o object
# files and *~ backup files:
#
clean: 
	$(RM) *.o *~
