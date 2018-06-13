cmake_minimum_required(VERSION 2.8)
project(ndpi-httpfilter)
set(EXE_NAME ndpi-httpfilter)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR}/deps/cmake)
set(CMAKE_C_FLAGS "--std=gnu99 ${CMAKE_C_FLAGS}")

aux_source_directory(./src DIR_SRCS)
set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})


include_directories(deps/zlog/src)
add_subdirectory(deps/zlog)
list(APPEND DEPS_LIB zlog)

include_directories(deps/nDPI/src/include)
add_subdirectory(deps/nDPI)
list(APPEND DEPS_LIB ndpi)


list(APPEND DEPS_LIB m)


add_executable(${EXE_NAME} ${DIR_SRCS})

#multiple threads
find_package(Threads REQUIRED) 
if (THREADS_HAVE_PTHREAD_ARG) 
	target_compile_options(PUBLIC ${EXE_NAME} "-pthread") 
endif () 
if (CMAKE_THREAD_LIBS_INIT)
	list(APPEND DEPS_LIB ${CMAKE_THREAD_LIBS_INIT}) 
endif ()

find_package(FindPCAP)
if (PCAP_FOUND)
	include_directories(${PCAP_INCLUDE_DIR})
	list(APPEND DEPS_LIB ${PCAP_LIBRARY})
else ()
	message(FATAL_ERROR "libpcap not found!")
endif ()

target_link_libraries(${EXE_NAME} ${DEPS_LIB})
