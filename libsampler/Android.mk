LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libsamplerlib
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false
LOCAL_ARM_MODE := arm

LOCAL_C_INCLUDES += $(LOCAL_PATH)/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libmlist/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libmqueue/include

LOCAL_CFLAGS := -fPIC -DSMPL_FALLBACK_VAL="NO_SIG" -DHAVE_ANDROID_OS
LOCAL_CFLAGS += -g3 -O0
LOCAL_CFLAGS += -DPARSER_REMOVE_EOLS
LOCAL_CFLAGS += -DMQ_NUMBER_OF_MESSAGES=1000
LOCAL_CFLAGS += -DCLOCK_MONOTONIC_RAW=4
ifdef SAMPLER_TMPDIR
    LOCAL_CFLAGS += -DSAMPLER_TMPDIR="${SAMPLER_TMPDIR}"
endif
#LOCAL_CFLAGS += -DVERBOSE_TYPE="2"

#Enable when code finalized
#LOCAL_CFLAGS += -DNDEBUG

#Enable only to check if a new target supports CTORS/DTORS
#mechanism (typically only once)
#LOCAL_CFLAGS += -DINITFINI_SHOW

LOCAL_SRC_FILES := \
   initfini.c \
   modglobals.c \
   sampler.c  \
   parse_setup.c \
   prepare.c \
   runengine.c \
   time.c \
   masters.c \
   master_tasks.c \
   workers.c \
   worker_tasks.c \
   eventgens.c \
   parsers.c \
   procsubst.c \
   assert_np.c

LOCAL_SHARED_LIBRARIES := \
   libmlist \
   libmqueue \

#LOCAL_LDLIBS += -lpthread
LOCAL_LDLIBS += -lm
#LOCAL_LDLIBS += -lrt

ifdef LIB_DYNAMIC
include $(BUILD_SHARED_LIBRARY)
else
include $(BUILD_STATIC_LIBRARY)
endif
$(call import-module,libmlist)
