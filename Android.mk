LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := btctl.c
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE := btctl

include $(BUILD_EXECUTABLE)
