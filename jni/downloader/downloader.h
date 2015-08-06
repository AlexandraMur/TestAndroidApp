#ifndef DOWNLOADER__H
#define DOWNLOADER__H
#include <jni.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct Downloader;
typedef struct Downloader Downloader;

enum DownloaderStatus { 
	DOWNLOADER_STATUS_ERROR = -1,
	DOWNLOADER_STATUS_OK = 0,
	DOWNLOADER_STATUS_CONT = -2,
	DOWNLOADER_STATUS_EXIT = -3
};

typedef struct {
	void (*complete)(Downloader *d, void *args, char *url, char *name_of_file, int status, size_t number_files_in_stack);
	void (*progress)(Downloader *d, void *args, char *url, char *name_of_file, int64_t curr_size, int64_t total_size);
} IDownloader_Cb;

Downloader* downloader_create(IDownloader_Cb*, void* args);
void downloader_destroy(Downloader*);
int downloader_add(Downloader *d, char* url, char* name_of_file);
int downloader_OnLoad(JavaVM *vm_);


#ifdef __cplusplus
}
#endif

#endif//DOWNLOADER__H

