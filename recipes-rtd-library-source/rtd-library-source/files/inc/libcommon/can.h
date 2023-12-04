#ifndef CAN_H
#define CAN_H
#include <stdint.h>
#include <linux/can.h>
#ifdef __cplusplus
extern "C" {
#endif
int can_init(const char *, int);
int set_can_mask_and_filter(uint32_t *mask, uint32_t *filter, int no_of_filter);
int can_write(char *, char *);
int can_read(char *, struct canfd_frame *);
int config_can_wakeup( char *can_name, int option );
int can_deinit(const char *);

int CAN_init();
int CAN_deinit();
int CAN_resp_recv(uint8_t *can_resp, uint8_t * length);
int CAN_request(uint16_t command);
int CAN_decode_dtc_resp(uint8_t *can_resp, uint8_t length, char * dtc_error);

#ifdef __cplusplus
}
#endif
#endif /* CAN_H */
