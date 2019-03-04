cmake_minimum_required(VERSION 3.5.1)

project(onvif)

include_directories(.)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ../../lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../../bin)

set(onvif_SOURCE
    soapC.c
    soapClient.c
    stdsoap2.c
    wsaapi.c
    duration.c
    dom.c
    mecevp.c
    smdevp.c
    threads.c
    wsseapi.c
)

add_library(onvif SHARED ${onvif_SOURCE})

target_compile_definitions(onvif
    PRIVATE -DWITH_DOM
    PRIVATE -DWITH_OPENSSL
)

target_link_libraries(onvif 
    ssl
    crypto
)