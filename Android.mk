LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SHARED_LIBRARIES := libmlist
LOCAL_SRC_FILES:= main.c sampler.c
LOCAL_MODULE := sampler
include $(BUILD_EXECUTABLE)


include $(CLEAR_VARS)
LOCAL_SRC_FILES:= mlist.c
LOCAL_MODULE := libmlist
include $(BUILD_SHARED_LIBRARY)
