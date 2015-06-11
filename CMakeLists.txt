cmake_minimum_required (VERSION 2.6)
project (sampler C)

################################################################################
# Configuration options
################################################################################
set(DEF_DEBUGFS_PATH
	"/sys/kernel/debug"
	CACHE STRING
	"Default path to where debugfs is mounted")

set(DEF_WORKDIR
	"./"
	CACHE STRING
	"Default workdir/where output is placed")

set(DEF_HTIME
	"1000"
	CACHE STRING
	"Default harvest-time (uS)")

set(DEF_PTIME
	"1000"
	CACHE STRING
	"Default period-time (uS)")

set(DEF_FTRACE_CLOCK
	"local"
	CACHE STRING
	"ftrace_clock to use. Note: Valid ones depend on target.")

set(DEF_FTRACE_BUFF_SIZE_KB
	"10000"
	CACHE STRING
	"Size of each CPU trace buffer in kB.")

#SET(BUILD_PARAVIEW_PLUGIN ON CACHE STRING "Build Paraview plugin?" FORCE)
set(SYSROOT
	${CMAKE_SYSROOT}
	CACHE STRING
	"System path (--system=)")

# Enable any liblog
option(ENABLE_LOGGING
    "Enable log messages." ON)

option(ENABLE_SYSLOG
    "Enable log to syslog (Linux/OSX)" OFF)
################################################################################
if (NOT SYSROOT STREQUAL "")
	message( "** INFO: SYSROOT was either set or defaulted from toolchain file" )
	set(CMAKE_SYSROOT "${SYSROOT}")
	set(CMAKE_EXTRA_C_FLAGS "${CMAKE_EXTRA_C_FLAGS} --sysroot=${CMAKE_SYSROOT}")
endif (NOT SYSROOT STREQUAL "")

set(CMAKE_C_FLAGS "${CMAKE_EXTRA_C_FLAGS} -no-integrated-cpp -Wno-unused-function -g3 -ggdb3 -O0 -Wall")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_EXTRA_C_FLAGS} -no-integrated-cpp -Wno-unused-function -g3 -ggdb3 -O0 -Wall")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_EXTRA_C_FLAGS} -no-integrated-cpp -Wno-unused-function -g3 -ggdb3 -O0 -Wall")


################################################################################
# Libraries (src binding)
################################################################################
include_directories ("${PROJECT_SOURCE_DIR}/lib/libsampler/include")
include_directories ("${PROJECT_SOURCE_DIR}/idoc/include")
include_directories ("${PROJECT_SOURCE_DIR}/lib/liblog/include")
include_directories ("${PROJECT_SOURCE_DIR}/lib/libmlist/include")
include_directories ("${PROJECT_SOURCE_DIR}/lib/libmtime/include")
include_directories ("${PROJECT_SOURCE_DIR}/lib/libmqueue/include")

# -sampler- Main bulk of the project is in this library
add_subdirectory (lib/libsampler)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--undefined=__sampler_init")
set (EXTRA_LIBS ${EXTRA_LIBS} libsampler)

# -Man-, interactive help- and other generated pages
add_subdirectory (idoc)
set (EXTRA_LIBS ${EXTRA_LIBS} idoc)

# -Logging and error/exception handling-
if (ENABLE_LOGGING)
    add_subdirectory (lib/liblog)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--undefined=__liblog_init")
	set (EXTRA_LIBS ${EXTRA_LIBS} liblog)
endif (ENABLE_LOGGING)

# -Lists-
add_subdirectory (lib/libmlist)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--undefined=__mlist_init")
set (EXTRA_LIBS ${EXTRA_LIBS} libmlist)

# -time-
add_subdirectory (lib/libmtime)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--undefined=__mtime_init")
set (EXTRA_LIBS ${EXTRA_LIBS} libmtime)

# -mqueue-
add_subdirectory (lib/libmqueue)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--undefined=__mqueue_init")
set (EXTRA_LIBS ${EXTRA_LIBS} libmqueue)

################################################################################
# external libraries
################################################################################
if (NOT ANDROID_TARGET)
	set (EXTRA_LIBS ${EXTRA_LIBS} cap)
endif (NOT ANDROID_TARGET)

################################################################################
# Project
################################################################################
# The version number.
set (sampler_VERSION_MAJOR 0)
set (sampler_VERSION_MINOR 0)
set (sampler_VERSION_PATCH 1)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/config.h.in"
  "${PROJECT_BINARY_DIR}/config.h"
  )

# add the binary tree to the search path for include files
# so that we will find TutorialConfig.h
include_directories("${PROJECT_BINARY_DIR}")

set(SAMPLER_SOURCE
	sampler.c
	opts.c
	opts_help.c
)

install(PROGRAMS ${PROJECT_BINARY_DIR}/sampler DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

add_executable(sampler ${SAMPLER_SOURCE})
target_link_libraries (sampler ${EXTRA_LIBS})
