LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := sampler
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false
LOCAL_ARM_MODE := arm
LOCAL_CFLAGS += -fPIC
LOCAL_CFLAGS += -DNDEBUG
ifdef SAMPLER_TMPDIR
    LOCAL_CFLAGS += -DSAMPLER_TMPDIR="${SAMPLER_TMPDIR}"
endif
ifndef LIB_DYNAMIC
LOCAL_LDFLAGS += -Wl,--undefined=__sampler_init -Wl,--undefined=__mlist_init
endif

LOCAL_C_INCLUDES += $(LOCAL_PATH)/libsampler/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/libmlist/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/libmqueue/include

LOCAL_SRC_FILES:= \
   main.c \
   doc.c

LOCAL_SHARED_LIBRARIES := \
   libmlist \
   libsamplerlib \
   libmqueue

#LOCAL_LDLIBS += mlist
#LOCAL_LDLIBS += sampler

include $(LOCAL_PATH)/common.mk

#Possible to build completely static in-source
#LOCAL_FORCE_STATIC_EXECUTABLE := true

include $(BUILD_EXECUTABLE)
$(call import-module,libmqueue)
$(call import-module,libmlist)
$(call import-module,libsampler)
