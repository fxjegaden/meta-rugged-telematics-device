#ifndef __LIBXML2__H
#define __LIBXML2__H 

#include <fcntl.h>	/* For O_* constants */
#include <sys/stat.h>	/* For mode constants */
#include <semaphore.h>
#include <errno.h>
#ifdef __cplusplus
extern "C" {
#endif
#define SEM_OPEN_FILE	"/sem_16"
#define SRC_XML_FILE	"/sample.xml"
sem_t *ret_value;

int get_xml_content(char *filename, char *parent_node,char *name, char *value);

int set_xml_content(char *filename, char *parent_node,char *name, char *value);

int sem_init_usb (const char *);
int sem_init_4g (const char *,sem_t *);
#ifdef __cplusplus
}
#endif
#endif /*__LIBXML2__H*/
