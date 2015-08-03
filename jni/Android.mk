LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_CFLAGS := -std=c99
LOCAL_MODULE := downloader
LOCAL_SRC_FILES := downloader/downloader.c
include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_CFLAGS := -std=c99
LOCAL_MODULE := cJSON
LOCAL_SRC_FILES := parser/cJSON.c
include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_CFLAGS += -std=c99
LOCAL_SHARE_LIBRARIES := ljson
LOCAL_MODULE := parser
LOCAL_SRC_FILES := parser/parser.c
include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_CFLAGS := -std=c99
LOCAL_MODULE := callbacks
LOCAL_SRC_FILES := callbacks.c
LOCAL_LDFLAGS := -llog
include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_CFLAGS := -std=c99
LOCAL_MODULE := test
LOCAL_SRC_FILES := test.c
LOCAL_LDFLAGS := -llog
include $(BUILD_SHARED_LIBRARY) 
