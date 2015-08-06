#ifdef USE_CURL
#include "http_client.h"
#include "curl/curl.h"

struct HttpClient {
	IHttpClientCb *cb;
};

static int progress_callback(void *_c, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow){
	HttpClient* c = (HttpClient*)_c;
	if (dlnow == 0 || dltotal ==0){
		return 0;
	}
	if (c->cb->progress){
		c->cb->progress(d, c->args, c->_entry->url, c->_entry->name_of_file, dlnow, dltotal);
	}
  	return 0;
}

static size_t callback(void *ptr, size_t size, size_t nmemb, Downloader* d){
    size_t _size = 0;
    if (d->alive){
           // _size = fwrite(ptr, size, nmemb, d->file); мы же в память сохраняем?!
    }
}

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

	//d->file = NULL;
	//d->file = fopen(d->_entry->name_of_file,"wb");

	//if(!d->file) {
		//goto exit;
	//}

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, c);
	curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
	curl_easy_setopt(curl, CURLOPT_XFERINFODATA, c);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
	CURLcode code = curl_easy_perform(curl);
	fclose(d->file);

	if(code){
		remove(d->_entry->name_of_file);
		error = DOWNLOADER_STATUS_ERROR;
	}
	error = DOWNLOADER_STATUS_OK;
exit:
	if (curl){
		curl_easy_cleanup(curl);
		curl = NULL;
	}
	if (d->my_callbacks->complete) {
		d->my_callbacks->complete(d, d->args, d->_entry->url, d->_entry->name_of_file, error, d->queue_size);
	}
}

void http_client_reset (HttpClient *c){
	//TODO
}

void http_client_destroy (HttpClient *c){
	free(c);
}

#endif //USE_CURL
