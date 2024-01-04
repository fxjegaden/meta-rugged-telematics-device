#ifndef __LIB_MCU_I2C_H__
#define __LIB_MCU_I2C_H__
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include "lib_battery.h"
#include "init.h"
#include "pm.h"
#include "lib_common.h"
#include "mcu_i2c_common.h"
#include "error_nos.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Debug Prints Enable */
#define COMMON_DEBUG_PRINTS			1
#define DEBUG_EN				0
#if DEBUG_EN
#define DEBUG_PRINTS				1
#define DEC_TO_HEX_DEBUG_EN			1
#define MCU_I2C_BUF_FRAME_DEBUG_EN		1
#define MCU_I2C_READ_DEBUG_EN			1
#define MCU_I2C_WRITE_DEBUG_EN			1
#endif

#define MCU_I2C_MAX_READ_TIMEOUT		5000
#define MAX_I2C_FRAME_SIZE			256

#define MCU_SLAVE_ADDRESS			0x76
#define MCU_BUS_ADDRESS				0
#define MCU_REG_ADDRESS				0xB0

#define MIN_I2C_FRAME_SIZE			10
#define MAX_I2C_FRAME_SIZE			256
#define LENGTH_BYTE_POSITION			2

/* i2c command frame position */
#define MCU_I2C_REG_ADDR_BYTE			0
#define MCU_I2C_CMD_ID_BYTE			1
#define MCU_I2C_CMD_LENGTH_BYTE			2
#define MCU_I2C_CMD_DATA_BYTE			3

/* i2c command decode frame position */
#define MCU_I2C_RES_ID_BYTE			0
#define MCU_I2C_RES_ERR_CODE_BYTE		1
#define MCU_I2C_RES_LENGTH_BYTE			5
#define MCU_I2C_RES_DATA_BYTE			6

#define I2C_FILE_NAME				"/dev/i2c-"

/* MCU Sleep Modes */
enum sleep_mode_id
{
	SLEEP_MODE = 0x01,
	DEEP_SLEEP_MODE,
	POWER_DOWN_MODE,
	DEEP_POWER_DOWN_MODE,
};

/* MCU Request Command IDs */
enum request_command_id
{
	CPU_MCU_SLEEP_REQUEST = 0x01,
	CPU_MCU_WAKEUP_SOURCE_REQUEST,
	CPU_MCU_FIRMWARE_VERSION_READ_REQUEST,
	CPU_MCU_KEEPALIVE_REQUEST,
	CPU_MCU_RS232_DATA_READ_REQUEST,
	CPU_MCU_RS232_DATA_WRITE_REQUEST,
	CPU_MCU_ADC_READ_REQUEST,
	CPU_MCU_DIN_READ_REQUEST,
	CPU_MCU_DOUT_WRITE_REQUEST,
	NUM_OF_REQUEST_COMMAND_ID,
};

/* MCU Response Command IDs */
enum response_command_id
{
	CPU_MCU_SLEEP_RESPONSE = 0x81,
	CPU_MCU_WAKEUP_SOURCE_RESPONSE,
	CPU_MCU_FIRMWARE_VERSION_READ_RESPONSE,
	CPU_MCU_KEEPALIVE_RESPONSE,
	CPU_MCU_RS232_DATA_READ_RESPONSE,
	CPU_MCU_RS232_DATA_WRITE_RESPONSE,
	CPU_MCU_ADC_READ_RESPONSE,
	CPU_MCU_DIN_READ_RESPONSE,
	CPU_MCU_DOUT_WRITE_RESPONSE,
	NUM_OF_RESPONSE_COMMAND_ID,
};

/* MCU related APIs */
/* To print the Framing and Decoded Structure contents */
void print_frame_details( i2c_cmd_frame i2c_buf_ptr );
void print_decoded_frame_details( i2c_cmd_decode i2c_buf_ptr );
/* API to convert decimal to hexadecimal */
int decToHex(uint32_t dec, uint8_t *hex);
/* API for i2c command framing */
int i2c_buf_frame( i2c_cmd_frame i2c_frame_buf, uint8_t *i2c_command );
/* API for Decoding the received i2c frame. This API fills the i2c_cmd_decode_struct structure */
int i2c_buf_decode( i2c_cmd_decode *i2c_decode_buf, uint8_t *i2c_command );
/* CPU to MCU i2c read and write APIs */
int set_i2c_register_values( uint32_t i2c_file, uint8_t i2c_addr, uint8_t *i2c_frame, uint32_t length );
int mcu_i2c_write( int bus_num, uint8_t i2c_addr, uint8_t *i2c_frame );
int get_i2c_register_values( uint32_t i2c_file, uint8_t i2c_addr, uint8_t *i2c_frame, uint32_t length );
int mcu_i2c_read( uint32_t bus_num, uint8_t i2c_addr, uint8_t *i2c_frame );
/* API for Enabling the requested Wakeup Sources */
int enable_requested_wakeup_sources( uint8_t wakeup_source, uint32_t *timer );
#ifdef __cplusplus
}
#endif
#endif /* __LIB_MCU_I2C_H__ */
