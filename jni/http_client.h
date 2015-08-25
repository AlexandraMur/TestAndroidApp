#ifndef HTTP_CLIENT_H_
#define HTTP_CLIENT_H_

#if ANDROID
#include <jni.h>
#endif //ANDROID

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

struct HttpClient;
typedef struct HttpClient HttpClient;

typedef enum {
    HTTP_CLIENT_OK,
	HTTP_CLIENT_STOP,
	HTTP_CLIENT_INSUFFICIENT_RESOURCE,
    HTTP_CLIENT_TIMEOUT_CONNECT,
    HTTP_CLIENT_TIMEOUT_RECIEVE,
	HTTP_CLIENT_BAD_URL,
	HTTP_CLIENT_ERROR
	// ...
} HttpClientStatus;

typedef struct {
    void (*data)(HttpClient *c, void *arg, const void *buffer, size_t size);
    void (*progress)(HttpClient *c, void *arg, int64_t total_size, int64_t curr_size);
} IHttpClientCb;

HttpClientStatus http_client_download (HttpClient *c, const char *url);
HttpClient* http_client_create (const IHttpClientCb *cb, void* args);
void http_client_destroy (HttpClient *c);
void http_client_reset (HttpClient *c);

void http_client_set_timeout_connection (HttpClient *, int);
void http_client_set_timeout_recieve (HttpClient *, int);
int http_client_get_timeout_connection (HttpClient *);
int http_client_get_timeout_recieve (HttpClient *);

#ifdef ANDROID
	int http_client_on_load (JavaVM *vm_);
	void http_client_android_detach (void);
#endif

#ifdef __cplusplus
}
#endif
#endif /* HTTP_CLIENT_H_ */
