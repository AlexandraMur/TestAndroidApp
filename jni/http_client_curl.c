#ifdef USE_CURL
#include "http_client.h"

static int progress_callback(void *_d, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow){
	Downloader* d = (Downloader*)_d;

	if (dlnow == 0 || dltotal ==0){
		return 0;
	}
	if (d->my_callbacks->progress){
		d->my_callbacks->progress(d, d->args, d->_entry->url, d->_entry->name_of_file, dlnow, dltotal);
	}
  	return 0;
}

static size_t callback(void *ptr, size_t size, size_t nmemb, Downloader* d){
    size_t _size = 0;
    if (d->alive){
            _size = fwrite(ptr, size, nmemb, d->file);
    }
    return _size;
}

HttpClient* http_client_create (HttpClient *c, void* args){
	//TODO
}

HttpClientStatus http_client_download (HttpClient *c, const char *url){
	int error = DOWNLOADER_STATUS_ERROR;
	CURL *curl = curl_easy_init();
	if (!curl) {
		goto exit;
	}

	d->file = NULL;
		d->file = fopen(d->_entry->name_of_file,"wb");

	if(!d->file) {
		goto exit;
	}

	curl_easy_setopt(curl, CURLOPT_URL, d->_entry->url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, d);
	curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
	curl_easy_setopt(curl, CURLOPT_XFERINFODATA, d);
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
	//TODO
}

#endif //USE_CURL
