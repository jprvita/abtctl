LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := libble-scan.c
LOCAL_SHARED_LIBRARIES := libble
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE := libble-scan

include $(BUILD_EXECUTABLE)
