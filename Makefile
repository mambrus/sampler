PHONIES=build all clean install uninstall
.DEFAULT_GOAL=all

ifneq (${MAKELEVEL},0)

LOCAL_PATH := .
include common.mk

LOCAL_MODULE := sampler
LOCAL_SRC_FILES := \
   doc.c \
   main.c

LOCAL_LIBS := \
   sampler \
   mqueue \
   mlist
ifndef LIB_DYNAMIC
LOCAL_LIBS += \
   pthread \
   rt \
   m \
   cap
endif

# Must be set or build will be sub-module order dependant, which can't be
# guaranteed for make -jn, n > 1 (see it's usage in submodule's Makefile)
export LOCAL_BUILD:=yes
LOCAL_CFLAGS := -fPIC
LOCAL_CFLAGS += -DNDEBUG
ifdef SAMPLER_TMPDIR
    LOCAL_CFLAGS += -DSAMPLER_TMPDIR="${SAMPLER_TMPDIR}"
endif
ifndef LIB_DYNAMIC
    LOCAL_CFLAGS += -Wl,--undefined=__sampler_init -Wl,--undefined=__mlist_init
endif
LOCAL_C_INCLUDES += libsampler/include libmlist/include libmqueue/include
LOCAL_LDLIBS += libsampler/lib libmlist/lib libmqueue/lib

LOCAL_SUBMODULES := \
   libmqueue \
   libmlist \
   libsampler

ifeq ($(LOCAL_CLANG),)
	CC=gcc
else
	CC=clang
endif

NDK_BUILD := ndk-build
NDK_VARIABLES := \
   APP_PLATFORM:=android-16 \
   $(if $(LOCAL_CLANG),NDK_TOOLCHAIN_VERSION=clang3.3) \
   NDK_PROJECT_PATH=$(CURDIR) \
   NDK_MODULE_PATH=$(CURDIR) \
   TARGET_ARCH_ABI=armeabi-v7a \
   APP_BUILD_SCRIPT=Android.mk

.PHONY: ${PHONIES} ${LOCAL_SUBMODULES}

WHOAMI := $(shell whoami)
ifeq (${WHOAMI},root)
    INSTALLDIR=/usr/local
else
    INSTALLDIR=${HOME}
endif

ifndef LIB_DYNAMIC
    ifeq (${LOCAL_BUILD},yes)
        $(LOCAL_MODULE): libsampler/lib/libsampler.a libmlist/lib/libmlist.a libmqueue/lib/libmqueue.a
    else
        $(LOCAL_MODULE): ${INSTALLDIR}/lib/libsampler.a ${INSTALLDIR}/lib/libmlist.a ${INSTALLDIR}/lib/libmqueue.a
    endif
else
#    ifeq (${LOCAL_BUILD},yes)
#        $(LOCAL_MODULE): libsampler/lib/libsampler.so libmlist/lib/libmlist.so libmqueue/lib/libmqueue.so
#    else
#        $(LOCAL_MODULE): ${INSTALLDIR}/lib/libsampler.so ${INSTALLDIR}/lib/libmlist.so ${INSTALLDIR}/lib/libmqueue.so
#    endif
endif

#--- Devs settings ends here. Rest is generic ---

EXHEADERS=$(shell ls include/*.h)
LOCAL_C_INCLUDES := $(addprefix -I , ${LOCAL_C_INCLUDES})
LOCAL_LDLIBS:= $(addprefix -L, ${LOCAL_LDLIBS})
LOCAL_LIBS:= ${LOCAL_LDLIBS} $(addprefix -l, ${LOCAL_LIBS})
LOCAL_AR_LIBS:= $(addprefix ${LOCAL_LDLIBS}/lib, ${LOCAL_AR_LIBS})
LOCAL_AR_LIBS:= $(addsuffix .a, ${LOCAL_AR_LIBS})
CFLAGS += -O0 -g3 ${LOCAL_CFLAGS} ${BUILD_CFLAGS}
CFLAGS += -I ./include ${LOCAL_C_INCLUDES}
CLEAN_MODS := $(patsubst %, $(MAKE) clean -C %;,$(LOCAL_SUBMODULES))
UNINST_MODS := $(patsubst %, $(MAKE) uninstall -C %;,$(LOCAL_SUBMODULES))
FOUND_CDEPS:= $(shell ls *.d 2>/dev/null)
RM := rm
.INTERMEDIATE: ${LOCAL_SRC_FILES:.c=.d}
#.SECONDARY: ${LOCAL_SRC_FILES:.c=.tmp}
#.INTERMEDIATE: ${LOCAL_SRC_FILES:.c=.tmp}

build: ${LOCAL_SUBMODULES} tags README.md $(LOCAL_MODULE)
all: build install

${LOCAL_SUBMODULES}:
	@if [ ! -h $@ ] && [ ! -d $@ ]; then \
		echo "Missing external dependency: $@" 1>&2; \
		echo "Please read the file BUILD in root-directory for details" 1>&2; \
		exit 1; \
	fi
	@echo "info: Diving into submodule: $@"
ifeq (${LOCAL_BUILD},yes)
	$(MAKE) -e -k -C $@
else
	$(MAKE) -e -k -C $@ install
endif

clean: common-clean
	$(CLEAN_MODS)
	rm -f *.o
	rm -f *.tmp
	rm -f *.d
	rm -f $(LOCAL_MODULE)
	rm -f tags

install: ${INSTALLDIR}/bin/${LOCAL_MODULE}

uninstall:
	$(UNINST_MODS)
	rm -rf ${INSTALLDIR}/bin/${LOCAL_MODULE}

${INSTALLDIR}/bin/${LOCAL_MODULE}: $(LOCAL_MODULE)
	mkdir -p ${INSTALLDIR}/bin
	rm -f ${INSTALLDIR}/bin/${LOCAL_MODULE}
	cp $(LOCAL_MODULE) ${INSTALLDIR}/bin/${LOCAL_MODULE}

tags: $(shell find . -name \"*.[chSs]\")
	ctags --options=.cpatterns --exclude=@.cexclude -o tags $$(find . -iname "*.[chs]")

$(LOCAL_MODULE): Makefile $(LOCAL_SRC_FILES:c=o)
	rm -f $(LOCAL_MODULE)
	${CC} $(CFLAGS) $(MODULE_FLAGS) $(LOCAL_SRC_FILES:c=o) ${LOCAL_LIBS} -o ${@}
ifdef LIB_DYNAMIC
    ifeq (${LIB_DYNAMIC},y)
        ifeq (${LOCAL_BUILD},yes)
			sh -c 'MSG=$(addprefix LD_LIBRARY_PATH=,${LOCAL_LIBS}) \
			 echo "warn: Remember for dev runs: export ${MSG}"'
        else
			@echo "warn: Remember for dev runs: export LD_LIBRARY_PATH=${INSTALLDIR}/lib"
       endif
    endif
endif
	@echo "info: >>>> Build $(LOCAL_MODULE) success! <<<<"


#Cancel out built-in implicit rule
%.o: %.c

%.tmp: %.c Makefile
	${CC} -MM $(CFLAGS) ${@:tmp=c} > ${@}

%.d: %.tmp
	cat ${@:d=tmp} | sed  -E 's,$*.c,$*.c $@,' > ${@}

%.o: %.d Makefile
	${CC} -c $(CFLAGS) ${@:o=c} -o $@

README_SCRIPT := \
    FS=$$(ls doc/* | sort); \
    for F in $$FS; do cat $$F | gawk -vRFILE=$$F '\
        BEGIN { \
            CHAPT=gensub("[_.[:alpha:]]*$$","","g",RFILE); \
            CHAPT=gensub("^.*/","","g",CHAPT); \
            CHAPT=gensub("_",".","g",CHAPT); \
        } \
        NR==1{ \
            S=sprintf("Chapter %s: %s",CHAPT,toupper($$0)); \
            printf("%s\n",S); \
            for (i=0; i< length(S); i++) \
                printf("="); \
            printf("\n"); \
            printf("\n"); \
        } \
        NR > 3{ \
            print $$0 \
        }'; \
    done

DOCFILES := $(shell ls doc/*)

README.md: ${DOCFILES}
	@rm ${@}
	@echo "Generaring file: README.md"
	@${README_SCRIPT} > ${@}

android:
	$(NDK_BUILD) $(NDK_VARIABLES)

android-clean: common-clean
	$(NDK_BUILD) $(NDK_VARIABLES) clean

include $(FOUND_CDEPS)

#========================================================================
else #First recursion level only executes this. Used for colorized output
#========================================================================

.PHONY: ${PHONIES} ${LOCAL_SUBMODULES}

ifeq ($(VIM),)
     HAS_GRCAT_BIN := $(shell which grcat)
endif

ifeq ($(HAS_GRCAT_BIN),)
    ifeq ($(VIM),)
        $(info No grcat installed. Build will not be colorized.)
    endif
    HAS_GRCAT=no
else
    HAS_GRCAT := $(shell if [ -f ~/.grc/conf.gcc ]; then echo "yes"; else echo "no"; fi)
    ifeq ($(HAS_GRCAT), no)
        $(warning NOTE: You have grcat installed, but no configuration file in for it (~/.grc/conf.gcc))
    endif
endif

${PHONIES} $(patsubst %.c,%.o,$(wildcard *.c)) tags README.md android android-clean:
ifeq ($(HAS_GRCAT), yes)
	( $(MAKE) $(MFLAGS) -e -C . $@ 2>&1 ) | grcat conf.gcc
else
	$(MAKE) $(MFLAGS) -e -C . $@
endif

endif

