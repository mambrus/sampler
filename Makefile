.PHONY: all clean install build

CFLAGS=-O0 -g3
LOCAL_MODULE := sampler
LOCAL_SRC_FILESS := \
   main.c \
   sampler.c \
   list.c

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

clean:
	rm -f *.o
	rm -f *.lib
	rm -f $(LOCAL_MODULE)
	rm -f tags

install: ${HOME}/bin/$(LOCAL_MODULE)

${HOME}/bin/$(LOCAL_MODULE): $(LOCAL_MODULE)
	rm -f ${HOME}/bin/$(LOCAL_MODULE)
	cp $(LOCAL_MODULE) ${HOME}/bin/$(LOCAL_MODULE)

tags: $(shell ls *.[ch])
	@ctags --options=.cpatterns --exclude=@.cexclude -o tags -R *

ifeq ($(HAS_GRCAT), yes)
$(LOCAL_MODULE): Makefile $(LOCAL_SRC_FILESS:c=o)
	@rm -f $(LOCAL_MODULE)
	@( gcc -o$(LOCAL_MODULE) $(CFLAGS) $(LOCAL_SRC_FILESS:c=o) -lpthread 2>&1 ) | grcat conf.gcc

%.o: %.c Makefile
	@( gcc -c $(CFLAGS) ${@:o=c} -o ${@} 2>&1 ) | grcat conf.gcc

else

$(LOCAL_MODULE): Makefile $(LOCAL_SRC_FILESS:c=o)
	@rm -f $(LOCAL_MODULE)
	@gcc -o$(LOCAL_MODULE) $(CFLAGS) $(LOCAL_SRC_FILESS:c=o) -lpthread

%.o: %.c Makefile
	@gcc -c $(CFLAGS) ${@:o=c} -o ${@}

endif
