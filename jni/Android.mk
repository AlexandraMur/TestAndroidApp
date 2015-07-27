LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := callbacks
LOCAL_SRC_FILES := callback.c

include $(BUILD_SHARED_LIBRARY)