#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define INPUT 1
#define OUTPUT 0

#define HIGH 1
#define LOW 0
#ifdef __cplusplus
extern "C" {
#endif

int GPIO_config(void);
int GPIO_off(void);
int gpio_export(int gpio, int direction);
int get_gpio_event(char *path, char *event);
int get_gpio(int gpio, int * value);
int set_gpio_direction(int gpio, int direction);
int set_gpio_value(int gpio, int value);
#ifdef __cplusplus
}
#endif