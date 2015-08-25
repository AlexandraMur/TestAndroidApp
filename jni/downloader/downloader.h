#ifndef DOWNLOADER__H
#define DOWNLOADER__H
#include <jni.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#ifdef __cplusplus
extern "C" {
#endif

struct Downloader;
typedef struct Downloader Downloader;

typedef enum {
	DOWNLOADER_STATUS_ERROR = -1,
	DOWNLOADER_STATUS_OK = 0,
	DOWNLOADER_STATUS_CONT = -2,
	DOWNLOADER_STATUS_EXIT = -3
} DownloaderStatus;

typedef struct {
	void (*complete)(Downloader *d, void *args, DownloaderStatus status, size_t number_files_in_stack);
	void (*progress)(Downloader *d, void *args, int64_t curr_size, int64_t total_size);
} IDownloader_Cb;

DownloaderStatus downloader_add(Downloader *d, const char *url, const char *name_of_file);
Downloader* downloader_create(const IDownloader_Cb*, void *args);
void downloader_set_timeout(Downloader*, int);
int downloader_get_timeout(Downloader*);
void downloader_destroy(Downloader*);
void downloader_stop(Downloader*);

#ifdef ANDROID
int downloader_OnLoad(JavaVM *vm);
#endif

#ifdef __cplusplus
}
#endif

#endif//DOWNLOADER__H
