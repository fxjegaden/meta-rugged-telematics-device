CFLAGS = -I../inc/libcommon/ -lRuggedTelematics -lm -ldl -lpthread

all:   mcu_test_app.o
	$(CC) $(CFLAGS) mcu_test_app.o -o mcu_test_app

mcu_test_app.o:  mcu_test_app.c
	$(CC) $(CFLAGS) -c mcu_test_app.c

# To start over from scratch, type 'make clean'. This
# removes the executable file, as well as old .o object
# files and *~ backup files:
#
clean: 
	$(RM) *.o *~
