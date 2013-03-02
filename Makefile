.PHONY: all clean install build

LOCAL_MODULE := sampler
LOCAL_SRC_FILESS := \
   main.c \
   sampler.c \

LOCAL_LIBS := \
   libmlist

LOCAL_CFLAGS :=
#LOCAL_CFLAGS := -DNDEBUG

CFLAGS=-O0 -g3 ${LOCAL_CFLAGS}

build: tags $(LOCAL_MODULE)
all: install

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

ifdef LIB_DYNAMIC
    $(info "Build will build and use it's libraries dynamic" )
    $(info "To build/use static libs: unset LIB_DYNAMIC" )
    LOCAL_LIBS := $(addsuffix .so, ${LOCAL_LIBS})
else
    $(info "Build will build and use it's libraries static" )
    $(info "To build all dynamic: export LIB_DYNAMIC='y'" )
    LOCAL_LIBS := $(addsuffix .a, ${LOCAL_LIBS})
endif

clean:
	rm -f *.o
	rm -f *.lib
	rm -f $(LOCAL_MODULE)
	rm -f tags
	rm -f *.so
	rm -f *.a

install: ${HOME}/bin/$(LOCAL_MODULE)

${HOME}/bin/$(LOCAL_MODULE): $(LOCAL_MODULE)
	rm -f ${HOME}/bin/$(LOCAL_MODULE)
	cp $(LOCAL_MODULE) ${HOME}/bin/$(LOCAL_MODULE)

tags: $(shell ls *.[ch])
	@ctags --options=.cpatterns --exclude=@.cexclude -o tags -R *

ifeq ($(HAS_GRCAT), yes)
$(LOCAL_MODULE): Makefile ${LOCAL_LIBS} $(LOCAL_SRC_FILESS:c=o)
	@rm -f $(LOCAL_MODULE)
	( gcc -o$(LOCAL_MODULE) $(CFLAGS) $(LOCAL_SRC_FILESS:c=o) -L`pwd` -lmlist -lpthread 2>&1 ) | grcat conf.gcc
	@echo $(LOCAL_MODULE) done...
	@echo If mlib build as shared lib remember to: export LD_LIBRARY_PATH=`pwd`

%.o: %.c Makefile
	( gcc -c $(CFLAGS) ${@:o=c} -o ${@} 2>&1 ) | grcat conf.gcc

libmlist.so: mlist.c Makefile
	( gcc $(CFLAGS) -shared mlist.c -o ${@} 2>&1 ) | grcat conf.gcc

libmlist.a: mlist.c Makefile
	( gcc -c $(CFLAGS) mlist.c -o ${@} 2>&1 ) | grcat conf.gcc

else

$(LOCAL_MODULE): Makefile ${LOCAL_LIBS} $(LOCAL_SRC_FILESS:c=o)
	@rm -f $(LOCAL_MODULE)
	@gcc -o$(LOCAL_MODULE) $(CFLAGS) $(LOCAL_SRC_FILESS:c=o) -L`pwd` -lmlist -lpthread
	@echo ">>>> Build $(LOCAL_MODULE) success! <<<<"

%.o: %.c Makefile
	@gcc -c $(CFLAGS) ${@:o=c} -o ${@}

libmlist.so: mlist.c Makefile
	@gcc $(CFLAGS) -shared mlist.c -o ${@} 2>&1

libmlist.a: mlist.c Makefile
	@gcc -c $(CFLAGS) mlist.c -o ${@} 2>&1

endif
