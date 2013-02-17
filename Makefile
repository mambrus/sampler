.PHONY: all clean install build

LOCAL_PROJECT := sampler

build: $(LOCAL_PROJECT)
all: install


HAS_GRCAT_BIN := $(shell which grcat)

ifeq ($(HAS_GRCAT_BIN),)
    $(info "No grcat installed. Build will not be colorized.")
    HAS_GRCAT=no
else
    HAS_GRCAT := $(shell if [ -f ~/.grc/conf.gcc ]; then echo "yes"; else echo "no"; fi)
    ifeq ($(HAS_GRCAT), no)
        $(warning "NOTE: you have grcat installed, but no configuration file in for it (~/.grc/conf.gcc)")
    endif
endif

clean:
	rm -f *.o
	rm -f *.lib
	rm -f $(LOCAL_PROJECT)

install: ${HOME}/bin/$(LOCAL_PROJECT)

${HOME}/bin/$(LOCAL_PROJECT): $(LOCAL_PROJECT)
	rm -f ${HOME}/bin/$(LOCAL_PROJECT)
	cp $(LOCAL_PROJECT) ${HOME}/bin/$(LOCAL_PROJECT)

ifeq ($(HAS_GRCAT), yes)
$(LOCAL_PROJECT): Makefile main.c
	@rm -f $(LOCAL_PROJECT)
	@( gcc -o$(LOCAL_PROJECT) -O0 -g3 main.c -lpthread 2>&1 ) | grcat conf.gcc

else

$(LOCAL_PROJECT): Makefile main.c
	@rm -f $(LOCAL_PROJECT)
	gcc -o$(LOCAL_PROJECT) -O0 -g3 main.c -lpthread


endif
