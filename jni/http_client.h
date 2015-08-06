#ifndef HTTP_CLIENT_H_
#define HTTP_CLIENT_H_

#include <jni.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

struct HttpClient;
typedef struct HttpClient HttpClient;

typedef enum {
    HTTP_CLIENT_OK,
    HTTP_CLIENT_TIMEOUT_CONNECT,
    HTTP_CLIENT_TIMEOUT_RECIEVE,
    HTTP_CLIENT_FAIL
} HttpClientStatus;

typedef struct {
    void (*data)(HttpClient *c, void *arg, const void *buffer, size_t size);
    void (*progress)(HttpClient *c, void *arg, int64_t total_size, int64_t curr_size);
} IHttpClientCb;

HttpClient* http_client_create (IHttpClientCb *cb, void* args);
HttpClientStatus http_client_download (HttpClient *c, const char *url);
void http_client_reset (HttpClient *c);
void http_client_destroy (HttpClient *c);

#ifdef ANDROID
	int http_client_on_load (JavaVM *vm_);
#endif

#ifdef __cplusplus
}
#endif
#endif /* HTTP_CLIENT_H_ */
