CFLAGS = -I../inc/rugged_telematics_lib/ -I../inc/libcommon/ -I../inc/libxml/ -lm -ldl -g -Wall -fPIC -lpthread -shared

all: 4g.o 4g_monitor.o acc_ism330dlc.o battery.o ble.o can_comm.o can_lib.o eth.o get.o gpio.o gyro_ism330dlc.o i2c_framing.o iio_utils.o init.o mag.o ntp.o power_management.o rs232.o serial.o thread_def.o u_gps.o wifi.o
	$(CC) $(CFLAGS) 4g.o 4g_monitor.o acc_ism330dlc.o battery.o ble.o can_comm.o can_lib.o eth.o get.o gpio.o gyro_ism330dlc.o i2c_framing.o iio_utils.o init.o mag.o ntp.o power_management.o rs232.o serial.o thread_def.o u_gps.o wifi.o -o libRuggedTelematics.so

4g.o: 4g.c
	$(CC) $(CFLAGS) -c 4g.c

4g_monitor.o: 4g_monitor.c
	$(CC) $(CFLAGS) -c 4g_monitor.c

acc_ism330dlc.o: acc_ism330dlc.c
	$(CC) $(CFLAGS) -c acc_ism330dlc.c

battery.o: battery.c
	$(CC) $(CFLAGS) -c battery.c

ble.o: ble.c
	$(CC) $(CFLAGS) -c ble.c

can_comm.o: can_comm.c 
	$(CC) $(CFLAGS) -c can_comm.c

can_lib.o: can_lib.c
	$(CC) $(CFLAGS) -c can_lib.c

eth.o: eth.c
	$(CC) $(CFLAGS) -c eth.c

get.o: get.c
	$(CC) $(CFLAGS) -c get.c

gpio.o: gpio.c
	$(CC) $(CFLAGS) -c gpio.c

gyro_ism330dlc.o: gyro_ism330dlc.c
	$(CC) $(CFLAGS) -c gyro_ism330dlc.c

i2c_framing.o: i2c_framing.c
	$(CC) $(CFLAGS) -c i2c_framing.c

iio_utils.o: iio_utils.c
	$(CC) $(CFLAGS) -c iio_utils.c

init.o: init.c
	$(CC) $(CFLAGS) -c init.c

mag.o: mag.c
	$(CC) $(CFLAGS) -c mag.c

ntp.o: ntp.c
	$(CC) $(CFLAGS) -c ntp.c

power_management.o: power_management.c
	$(CC) $(CFLAGS) -c power_management.c

rs232.o: rs232.c
	$(CC) $(CFLAGS) -c rs232.c

serial.o: serial.c
	$(CC) $(CFLAGS) -c serial.c

thread_def.o: thread_def.c
	$(CC) $(CFLAGS) -c thread_def.c

u_gps.o: u_gps.c
	$(CC) $(CFLAGS) -c u_gps.c

wifi.o: wifi.c
	$(CC) $(CFLAGS) -c wifi.c

# To start over from scratch, type 'make clean'. This
# removes the executable file, as well as old .o object
# files and *~ backup files:
#
clean: 
	$(RM) *.o *.so *~
