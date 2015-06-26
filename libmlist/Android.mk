LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libmlist
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false
LOCAL_ARM_MODE := arm

LOCAL_C_INCLUDES += $(LOCAL_PATH)/include

LOCAL_CFLAGS += -fPIC

#Enable when code finalized
#LOCAL_CFLAGS += -DNDEBUG

#Enable only to check if a new target supports CTORS/DTORS
#mechanism (typically only once)
#LOCAL_CFLAGS += -DINITFINI_SHOW

LOCAL_SRC_FILES := \
   modglobals.c \
   initfini.c \
   mlist.c

LOCAL_SHARED_LIBRARIES :=

ifdef LIB_DYNAMIC
include $(BUILD_SHARED_LIBRARY)
else
include $(BUILD_STATIC_LIBRARY)
endif
