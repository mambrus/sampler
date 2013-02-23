LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= main.c sampler.c list.c
LOCAL_MODULE := sampler
include $(BUILD_EXECUTABLE)
