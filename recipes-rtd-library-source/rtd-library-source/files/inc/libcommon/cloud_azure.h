#ifndef __CLOUD_AZURE_H_
#define __CLOUD_AZURE_H_

#include "openssl/ssl.h"
#include "openssl/err.h"
#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>
#include <curl/curl.h>

typedef struct _a_header{
char Accept[30];
char Content_Type[64];
char iothub_to[128];
char Authorization[256];
char Connection[64];
char User_Agent[128];
char iothub_app_temperatureAlert[64];
char Host[64];
char Content_Length[30];
}HTTP_HEADER;

HTTP_HEADER a_header;

typedef struct config_azure{
        char deviceid[64];
        char sas[256];
        char hostname[128];
        char url[128];
        char http_deviceId[64];
        char http_deviceType[64];
        char IoTHubName[64];
        CURL *curl;
        int o_init_state;
        int port;

}config_azure;

config_azure cfg_azure;

int send_data_to_cloud(char * buffer);
int frame_actual_header(HTTP_HEADER *packet, config_azure *cfg);
int cloud_deinit();
int cloud_init ();
int create_url(config_azure *cfg);
int read_cloud_config ();
int write_cloud_config (char *host, char *port, char *deviceId, char *deviceType, char *IoTHubName, char *sas);
static size_t HandleResponse(void *Ptr, size_t Size, size_t NoElements, void* Data);
size_t RecvResponseCallback ( char *ptr, size_t size, size_t nmemb, char *data );
#endif
