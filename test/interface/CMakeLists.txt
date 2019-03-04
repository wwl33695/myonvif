cmake_minimum_required(VERSION 3.5.1)

project(onvif-stream)

set(CMAKE_BUILD_TYPE Debug)

set(OPENSSL_INCLUDE /usr/include)
set(OPENSSL_LIB /usr/lib)

set(FFMPEG_INCLUDE /usr/include)
set(FFMPEG_LIB /usr/lib)

include_directories(.)
include_directories(onvif)
include_directories(${OPENSSL_INCLUDE})
include_directories(${FFMPEG_INCLUDE})

link_directories(${OPENSSL_LIB})
link_directories(${FFMPEG_LIB})

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ../lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../bin)

#######################################################################

set(onvif-stream_SOURCE
    onvif_comm.c
    onvif_dump.c
    rtsp_stream.cpp
)

add_library(onvif-stream SHARED ${onvif-stream_SOURCE})

#target_compile_definitions(onvif-stream
#    PRIVATE -D__STDC_CONSTANT_MACROS
#)

target_link_libraries(onvif-stream
    onvif avcodec avformat avutil swscale
)

#######################################################################

set(onvif-main_SOURCE
    main.cpp
)

add_executable(onvif-main ${onvif-main_SOURCE})

target_link_libraries(onvif-main 
    onvif-stream
)


ADD_SUBDIRECTORY(onvif)
