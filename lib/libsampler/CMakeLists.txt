INCLUDE (CheckLibraryExists)
include_directories("${PROJECT_BINARY_DIR}")
include_directories("${PROJECT_SOURCE_DIR}")

set(LIBSAMPLER_SOURCE
	samplerd.c   
	modglobals.c    
	parse_setup.c  
	parsers.c
	prepare.c
	procsubst.c
	master_tasks.c 
	runengine.c
	workers.c
	eventgens.c  
	masters.c   
	worker_tasks.c
)

if (ENABLE_LIBSAMPLER)
	set(LIBSAMPLER_SOURCE
		${LIBSAMPLER_SOURCE}
		initfini.c
	)
	set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--undefined=__sampler_init")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--undefined=__sampler_init")
endif (ENABLE_LIBSAMPLER)

set(LIBSAMPLER_SOURCE
	${LIBSAMPLER_SOURCE}
)

add_library(libsampler ${LIBSAMPLER_SOURCE})

CHECK_LIBRARY_EXISTS(cap capset "" HAVE_LIB_CAP)
if (HAVE_LIB_CAP)
    target_link_libraries(libsampler cap)
else (HAVE_LIB_CAP)
    target_link_libraries(libsampler)
endif (HAVE_LIB_CAP)
