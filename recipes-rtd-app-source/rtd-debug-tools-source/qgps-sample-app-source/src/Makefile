CFLAGS = -I../inc/libcommon/ -lRuggedTelematics -lm -ldl -g -Wall -fPIC -lpthread

all: qgps_test_application.o
	$(CC) $(CFLAGS) qgps_test_application.o -o QGPS_APP

qgps_test_application.o: qgps_test_application.c
	$(CC) $(CFLAGS) -c qgps_test_application.c

# To start over from scratch, type 'make clean'. This
# removes the executable file, as well as old .o object
# files and *~ backup files:
#
clean: 
	$(RM) *.o *~
