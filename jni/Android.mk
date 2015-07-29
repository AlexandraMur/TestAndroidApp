LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := callbacks
LOCAL_SRC_FILES := callbacks.c

LOCAL_MODULE := test
LOCAL_SRC_FILES := test.c

LOCAL_LDFLAGS := -llog

include $(BUILD_SHARED_LIBRARY) 
