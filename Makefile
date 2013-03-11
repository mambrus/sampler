PHONIES=build all clean install uninstall

ifneq (${MAKELEVEL},0)

LOCAL_MODULE := sampler
LOCAL_SRC_FILESS := \
   main.c

LOCAL_LIBS := \
   sampler \
   mlist

LOCAL_CFLAGS := -fPIC
LOCAL_CFLAGS += -DNDEBUG
LOCAL_C_INCLUDES += ${HOME}/include
LOCAL_LDLIBS += ${HOME}/lib
LOCAL_SUBMODULES := \
   libmlist \
   libsampler

.PHONY: ${PHONIES} ${LOCAL_SUBMODULES}

#--- Devs settings ends here. Rest is generic ---
WHOAMI := $(shell whoami)
ifeq (${WHOAMI},root)
    INSTALLDIR=/usr/local
else
    INSTALLDIR=${HOME}
endif
EXHEADERS=$(shell ls include/*.h)
LOCAL_C_INCLUDES := $(addprefix -I , ${LOCAL_C_INCLUDES})
LOCAL_LDLIBS:= $(addprefix -L, ${LOCAL_LDLIBS})
LOCAL_LIBS:= ${LOCAL_LDLIBS} $(addprefix -l, ${LOCAL_LIBS})
LOCAL_AR_LIBS:= $(addprefix ${LOCAL_LDLIBS}/lib, ${LOCAL_AR_LIBS})
LOCAL_AR_LIBS:= $(addsuffix .a, ${LOCAL_AR_LIBS})
CFLAGS += -O0 -g3 ${LOCAL_CFLAGS}
CFLAGS += -I ./include ${LOCAL_C_INCLUDES}
CLEAN_MODS := $(patsubst %, $(MAKE) clean -C %;,$(LOCAL_SUBMODULES))
UNINST_MODS := $(patsubst %, $(MAKE) uninstall -C %;,$(LOCAL_SUBMODULES))

build: ${LOCAL_SUBMODULES} tags README.md $(LOCAL_MODULE)
all: install

${LOCAL_SUBMODULES}:
	echo "Diving into submodule: $@"
	$(MAKE) -e -k -C $@ install

clean:
	$(CLEAN_MODS)
	rm -f *.o
	rm -f $(LOCAL_MODULE)
	rm -f tags

install: ${INSTALLDIR}/bin/${LOCAL_MODULE}

uninstall:
	$(UNINST_MODS)
	rm -rf ${INSTALLDIR}/bin/${LOCAL_MODULE}

${INSTALLDIR}/bin/${LOCAL_MODULE}: $(LOCAL_MODULE)
	mkdir -p ${INSTALLDIR}/bin
	rm -f ${INSTALLDIR}/${LOCAL_MODULE}
	cp $(LOCAL_MODULE) ${INSTALLDIR}/${LOCAL_MODULE}

tags: $(shell ls *.[ch])
	ctags --options=.cpatterns --exclude=@.cexclude -o tags -R *

$(LOCAL_MODULE): Makefile $(LOCAL_SRC_FILESS:c=o)
	rm -f $(LOCAL_MODULE)
	gcc $(CFLAGS) $(MODULE_FLAGS) $(LOCAL_SRC_FILESS:c=o) ${LOCAL_LIBS} -o ${@}
	@echo "Remember for dev runs: export LD_LIBRARY_PATH=${INSTALLDIR}/lib"
	@echo ">>>> Build $(LOCAL_MODULE) success! <<<<"

%.o: %.c Makefile
	gcc -c $(CFLAGS) ${@:o=c} -o ${@}

README_SCRIPT := \
    FS=$$(ls doc/* | sort); \
    for F in $$FS; do cat $$F | awk -vRFILE=$$F '\
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
	@echo "Generaring file: README.md"
	@${README_SCRIPT} > ${@}

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

${PHONIES} $(patsubst %.c,%.o,$(wildcard *.c)) tags README.md:
ifeq ($(HAS_GRCAT), yes)
	( $(MAKE) $(MFLAGS) -e -C . $@ 2>&1 ) | grcat conf.gcc
else
	$(MAKE) $(MFLAGS) -e -C . $@
endif

endif

