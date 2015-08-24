#include "downloader.h"
#include "http_client.h"

#include <unistd.h>
#include <pthread.h>
#include <sys/queue.h>

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>

#ifdef ANDROID
#include <android/log.h>
#define LOG_TAG	"downloader"
#define LOGI(fmt, ...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, "%s: " fmt, __func__, ## __VA_ARGS__)
#define LOGW(fmt, ...) __android_log_print(ANDROID_LOG_WARN,  LOG_TAG, "%s: " fmt, __func__, ## __VA_ARGS__)
#define LOGE(fmt, ...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "%s: " fmt, __func__, ## __VA_ARGS__)
#else
# include <stdio.h>
# define LOGI(fmt, ...) ((void)printf("(I): %s: " fmt, __func__ , ## __VA_ARGS__))
# define LOGE(fmt, ...) ((void)printf("(E): %s: " fmt, __func__ , ## __VA_ARGS__))
# define LOGW(fmt, ...) ((void)printf("(W): %s: " fmt, __func__ , ## __VA_ARGS__))
#endif

typedef struct Job
{
	TAILQ_ENTRY(Job) next;
    char *url;
    char *file_name;
} Job;

typedef TAILQ_HEAD(Jobs, Job) Jobs;

struct Downloader
{
	pthread_mutex_t mutex;
	pthread_cond_t cv;
	pthread_t thread;
	bool thread_initialized;
	bool mutex_initialized;
	bool cv_initialized;

	int timeout;
	int shutdown;
	HttpClient *http_client;

	Jobs jobs;
	size_t jobs_count;
	Job *current_job;

	const IDownloader_Cb *cb;
	void *arg;

	FILE *file;

#ifdef ANDROID
	JavaVM *vm;
#endif
};

#ifdef ANDROID
	JavaVM *g_vm;
#endif

static void data_cb (HttpClient *c, void *arg, const void *buffer, size_t size);
static void progress_cb (HttpClient *c, void *arg, int64_t total_size, int64_t curr_size);

static const IHttpClientCb kHttpClientCb =
{
	.data = data_cb,
	.progress = progress_cb
};

static void data_cb (HttpClient *c, void *arg, const void *buffer, size_t size)
{
	Downloader *d = (Downloader*)arg;
	assert(d);
	assert(d->file);

	pthread_mutex_lock(&d->mutex);
	if (d->shutdown){
		LOGI("SD");
		pthread_mutex_unlock(&d->mutex);
		return;
	}
	pthread_mutex_unlock(&d->mutex);
	if (buffer && size) {
		fwrite(buffer, 1, size, d->file);
	}
}

static void progress_cb (HttpClient *c, void *arg, int64_t total_size, int64_t curr_size)
{
	Downloader *d = (Downloader*)arg;
	assert(d);
	pthread_mutex_lock(&d->mutex);
	if (d->shutdown){
		pthread_mutex_unlock(&d->mutex);
		return;
	}
	pthread_mutex_unlock(&d->mutex);
	if (d->cb->progress) {
		d->cb->progress(d, d->arg, curr_size, total_size);
	}
}

static void job_destroy(Job *job)
{
	if (!job) {
		return;
	}
	if (job->url) {
		free(job->url);
		job->url = NULL;
	}
	if (job->file_name) {
		free(job->file_name);
		job->file_name = NULL;
	}
	free(job);
}

static Job* job_create (const char *url, const char *file_name)
{
	Job *job = calloc(1, sizeof(*job));
	if (!job) {
		goto fail;
	}
	job->url = strdup(url);
	if (!job->url) {
		goto fail;
	}
	job->file_name = strdup(file_name);
	if (!job->file_name) {
		goto fail;
	}
	return job;
fail:
	job_destroy(job);
	return NULL;
}

static void put_job (Downloader *d, Job *job)
{
	assert(job);

	TAILQ_INSERT_TAIL(&d->jobs, job, next);
	d->jobs_count++;
}

static Job* get_job (Downloader *d)
{
	assert(d->jobs_count);

	Job *job = TAILQ_FIRST(&d->jobs);
	TAILQ_REMOVE(&d->jobs, job, next);
	d->jobs_count--;
	return job;
}

static void clear_jobs (Downloader *d)
{
	while (TAILQ_FIRST(&d->jobs)) {
		Job *job = get_job(d);
		job_destroy(job);
	}
	assert(d->jobs_count == 0);
}

static DownloaderStatus httpclient_to_downloader_status(HttpClientStatus status)
{
	switch (status) {
		case HTTP_CLIENT_OK:
			return DOWNLOADER_STATUS_OK;
		default:
			return DOWNLOADER_STATUS_ERROR;
	}
}

static void *worker_thread (void *arg)
{
	Downloader* d = (Downloader*)arg;
	http_client_set_timeout(d->http_client, d->timeout);
    while (1) {
		pthread_mutex_lock(&d->mutex);
        while (TAILQ_EMPTY(&d->jobs) && !d->shutdown){
			pthread_cond_wait(&d->cv, &d->mutex);
        }
        if (d->shutdown) {
        	http_client_reset(d->http_client);
        	pthread_mutex_unlock(&d->mutex);
            break;
        }
   		d->current_job = get_job(d);
		pthread_mutex_unlock(&d->mutex);

		HttpClientStatus status = HTTP_CLIENT_INSUFFICIENT_RESOURCE;
		d->file = fopen(d->current_job->file_name, "wb");
		if (!d->file) {
			LOGE("Can't create file %s\n", d->current_job->file_name);
		} else {
			status = http_client_download(d->http_client, d->current_job->url);
			if (httpclient_to_downloader_status(status) != DOWNLOADER_STATUS_OK){
				remove(d->current_job->file_name);
			}
		}

		fclose(d->file);
		LOGI("Download complete, status == %d", status);

		if (d->cb->complete) {
			d->cb->complete(d, d->arg, httpclient_to_downloader_status(status), d->jobs_count);
		}
		job_destroy(d->current_job);
		d->current_job = NULL;
    }
#if ANDROID
    http_client_android_detach();
#endif
    return NULL;
}

Downloader *downloader_create(const IDownloader_Cb *cb, void *arg)
{
	if (!cb) {
		return NULL;
	}
	Downloader *d = calloc(1, sizeof(*d));
	if (!d) {
		goto fail;
	}
	TAILQ_INIT(&d->jobs);
	d->current_job = NULL;
	d->timeout = 0; // or define value?
	d->jobs_count = 0;
	d->shutdown = 0;
	d->arg = arg;
	d->vm = g_vm;
	d->cb = cb;

	d->http_client = http_client_create(&kHttpClientCb, d);
	if (!d->http_client) {
		goto fail;
	}
	if (pthread_mutex_init(&d->mutex, NULL)) {
		goto fail;
	}
	d->mutex_initialized = true;
	if (pthread_cond_init(&d->cv, NULL)) {
		goto fail;
	}
    d->cv_initialized = true;
    if (pthread_create(&d->thread, NULL, worker_thread, d)) {
    	goto fail;
    }
	d->thread_initialized = true;
	return d;
fail:
	downloader_destroy(d);
	return NULL;
} 

void downloader_destroy(Downloader *d)
{
	if (!d) {
		return;
	}

	if (d->thread_initialized) {
		pthread_mutex_lock(&d->mutex);
		d->shutdown = true;
		pthread_cond_broadcast(&d->cv);
		pthread_mutex_unlock(&d->mutex);
		pthread_join(d->thread, NULL);
	}
	if(d->mutex_initialized){
		pthread_mutex_destroy(&d->mutex);
	}
	if(d->cv_initialized){
		pthread_cond_destroy(&d->cv);
	}
	if (d->http_client) {
		http_client_destroy(d->http_client);
		d->http_client = NULL;
	}
	clear_jobs(d);
	assert(d->current_job == NULL);
	free(d);
}

int downloader_add(Downloader *d, const char *url, const char *file_name)
{
	if (!d) {
		return DOWNLOADER_STATUS_ERROR;
	}

	Job *job = job_create(url, file_name);
	if (!job) {
		return DOWNLOADER_STATUS_ERROR;
	}
	pthread_mutex_lock(&d->mutex);
	LOGI("Put job: %s %s\n", job->url, job->file_name);
	put_job(d, job);
	pthread_cond_broadcast(&d->cv);
	pthread_mutex_unlock(&d->mutex);
	return DOWNLOADER_STATUS_OK;
}

#if defined(ANDROID) && !defined(USE_CURL)
int downloader_OnLoad(JavaVM *vm)
{
	g_vm = vm;
	return http_client_on_load(vm);
}
#endif //defined(ANDROID) && !defined(USE_CURL)

void downloader_set_timeout(Downloader *d, int timeout)
{
	d->timeout = timeout;
};

int downloader_get_timeout(Downloader *d)
{
	return d->timeout;
};

#if defined(ANDROID) && !defined(USE_CURL)
void downloader_stop(void* d_)
{
	Downloader *d = (Downloader*) d_;
	assert(d);
	if (!d){
		return;
	}
	http_client_reset(d->http_client);
}
#endif //defined(ANDROID) && !defined(USE_CURL)
