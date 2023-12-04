#ifndef __DEBUG__H
#define __DEBUG__H
#include <errno.h>
//#include "error_macro.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ENABLE_OBD_DEBUG_ALL
//#define ENABLE_IOBD_DEBUG_LEVEL1
//#define ENABLE_IOBD_DEBUG_LEVEL2
//#define ENABLE_IOBD_DEBUG_LEVEL3
#define ERROR_FILE stderr

#define OBD2_APP_SUCCESS     (0)
#define OBD2_APP_FAILURE     (-1)

#define IOBD_ERR(...)\
        {\
        fprintf(stderr, "ERROR : %s L#%d ", __func__, __LINE__);  \
        fprintf(stderr,__VA_ARGS__); \
        fprintf(stderr,"\n"); \
        }  

#ifdef ENABLE_OBD_DEBUG_ALL
#define ENABLE_IOBD_DEBUG_LEVEL1
#define ENABLE_IOBD_DEBUG_LEVEL2
#define ENABLE_IOBD_DEBUG_LEVEL3
#endif

#ifdef ENABLE_IOBD_DEBUG_LEVEL1
#define IOBD_DEBUG_LEVEL1(...)\
{\
        printf("DEBUG:   %s L#%d ", __func__, __LINE__);  \
        printf(__VA_ARGS__); \
        printf("\n"); \
}
#else
#define IOBD_DEBUG_LEVEL1(...)
#endif

#ifdef ENABLE_IOBD_DEBUG_LEVEL2
#define IOBD_DEBUG_LEVEL2(...)\
{\
        printf(__VA_ARGS__); \
        printf("\n"); \
}
#else
#define IOBD_DEBUG_LEVEL2(...)
#endif

#ifdef ENABLE_IOBD_DEBUG_LEVEL3
#define IOBD_DEBUG_LEVEL3(...)\
{\
        printf("DEBUG:   %s L#%d ", __func__, __LINE__);  \
        printf(__VA_ARGS__); \
        printf("\n"); \
}
#else
#define IOBD_DEBUG_LEVEL3(...)
#endif


static inline int CHK_EOF (int ret, FILE *fd, char *err_str)
{
        if (ret <  OBD2_APP_SUCCESS) {
                fprintf (fd, "%s: %d\n", err_str, ret);
                return ret; 
        }
	return OBD2_APP_SUCCESS;
}

static inline int CHK_ERR ( int ret, FILE *fd, char *err_str) 
{
        if (ret != OBD2_APP_SUCCESS) {
                fprintf (fd, "%s: %d: ", err_str, errno);
		perror(NULL);
                return ret;
        }
	return OBD2_APP_SUCCESS;
}

static inline int CHK_NULL ( int *ret, FILE *fd, char *err_str) 
{
        if (ret == (int *)NULL) {
                fprintf (fd, "%s: %d\n", err_str, errno);
                return errno;
        }
	return OBD2_APP_SUCCESS;
}

#ifdef __cplusplus
}
#endif
#endif //__DEBUG__H
