LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_CFLAGS += -std=gnu99
LOCAL_MODULE := httpclient
LOCAL_SRC_FILES := http_client_android.c http_client_curl.c
LOCAL_LDFLAGS += -llog
LOCAL_LDLAGS += -DANDROID
include $(BUILD_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_CFLAGS += -std=gnu99
LOCAL_MODULE := downloader
LOCAL_SRC_FILES := downloader/downloader.c
LOCAL_STATIC_LIBRARIES += libhttpclient
LOCAL_CFLAGS += -llog
LOCAL_CFLAGS += -DANDROID
include $(BUILD_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_CFLAGS += -std=gnu99
LOCAL_MODULE := cJSON
LOCAL_SRC_FILES := parser/cJSON.c
LOCAL_CFLAGS += -DANDROID
include $(BUILD_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_CFLAGS += -std=gnu99
LOCAL_MODULE := parser
LOCAL_SRC_FILES := parser/parser.c
LOCAL_STATIC_LIBRARIES += cJSON
LOCAL_CFLAGS += -DANDROID
include $(BUILD_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_CFLAGS += -std=gnu99
LOCAL_MODULE := test
LOCAL_SRC_FILES := test.c
LOCAL_STATIC_LIBRARIES += libparser
LOCAL_STATIC_LIBRARIES += libdownloader
LOCAL_LDFLAGS += -llog
LOCAL_LDFLAGS += -DANDROID
include $(BUILD_SHARED_LIBRARY) 
  