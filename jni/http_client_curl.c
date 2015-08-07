#ifdef USE_CURL
#include "http_client.h"
#include "curl/curl.h"

struct HttpClient {
	IHttpClientCb *cb;
};

HttpClient* http_client_create (IHttpClientCb *cb, void* args){
	HttpClient *c = calloc(1, sizeof(struct HttpClient));
	c->cb = cb;
	return c;
}

HttpClientStatus http_client_download (HttpClient *c, const char *url){
	int error = DOWNLOADER_STATUS_ERROR;
	CURL *curl = curl_easy_init();
	if (!curl) {
		goto exit;
	}

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, c->data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, c);
	curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, c->progress);
	curl_easy_setopt(curl, CURLOPT_XFERINFODATA, c);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
	CURLcode code = curl_easy_perform(curl);

	if(code){
		error = DOWNLOADER_STATUS_ERROR;
		goto exit;
	}
	error = DOWNLOADER_STATUS_OK;

exit:
	if (curl){
		curl_easy_cleanup(curl);
		curl = NULL;
	}
}

void http_client_reset (HttpClient *c){
	//TODO
}

void http_client_destroy (HttpClient *c){
	free(c);
}

#endif //USE_CURL
