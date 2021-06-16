cmake_minimum_required(VERSION 3.2)
project(handy)

set(CMAKE_CXX_STANDARD 11)

include(GNUInstallDirs)

find_package(Threads REQUIRED)

list(APPEND HANDY_SRCS
    ${PROJECT_SOURCE_DIR}/handy/daemon.cc
    ${PROJECT_SOURCE_DIR}/handy/net.cc
    ${PROJECT_SOURCE_DIR}/handy/codec.cc
    ${PROJECT_SOURCE_DIR}/handy/http.cc
    ${PROJECT_SOURCE_DIR}/handy/conn.cc
    ${PROJECT_SOURCE_DIR}/handy/poller.cc
    ${PROJECT_SOURCE_DIR}/handy/udp.cc
    ${PROJECT_SOURCE_DIR}/handy/threads.cc
    ${PROJECT_SOURCE_DIR}/handy/file.cc
    ${PROJECT_SOURCE_DIR}/handy/util.cc
    ${PROJECT_SOURCE_DIR}/handy/conf.cc
    ${PROJECT_SOURCE_DIR}/handy/stat-svr.cc
    ${PROJECT_SOURCE_DIR}/handy/port_posix.cc
    ${PROJECT_SOURCE_DIR}/handy/event_base.cc
    ${PROJECT_SOURCE_DIR}/handy/logging.cc)

if(CMAKE_HOST_APPLE)
    add_definitions(-DOS_MACOSX)
elseif(CMAKE_HOST_UNIX)
    add_definitions(-DOS_LINUX)
else(CMAKE_HOST_APPLE)
    message(FATAL_ERROR "Platform not supported")
endif(CMAKE_HOST_APPLE)

option(BUILD_HANDY_SHARED_LIBRARY "Build Handy Shared Library" ON)
option(BUILD_HANDY_STATIC_LIBRARY "Build Handy Shared Library" ON)
option(BUILD_HANDY_EXAMPLES "Build Handy Examples" OFF)

##Handy Shared Library
if(BUILD_HANDY_SHARED_LIBRARY)
    add_library(handy SHARED ${HANDY_SRCS})
    target_include_directories(handy PUBLIC ${PROJECT_SOURCE_DIR}/handy)
    target_link_libraries(handy Threads::Threads)
    install(TARGETS handy DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif(BUILD_HANDY_SHARED_LIBRARY)

#Handy Static library
if(BUILD_HANDY_STATIC_LIBRARY)
    add_library(handy_s STATIC ${HANDY_SRCS})
    target_include_directories(handy_s PUBLIC ${PROJECT_SOURCE_DIR}/handy/)
    target_link_libraries(handy_s Threads::Threads)
    install(TARGETS handy_s DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif(BUILD_HANDY_STATIC_LIBRARY)

if(BUILD_HANDY_SHARED_LIBRARY OR BUILD_HANDY_STATIC_LIBRARY)
    install(FILES 
        ${PROJECT_SOURCE_DIR}/handy/codec.h
        ${PROJECT_SOURCE_DIR}/handy/conf.h
        ${PROJECT_SOURCE_DIR}/handy/conn.h
        ${PROJECT_SOURCE_DIR}/handy/daemon.h
        ${PROJECT_SOURCE_DIR}/handy/event_base.h
        ${PROJECT_SOURCE_DIR}/handy/file.h
        ${PROJECT_SOURCE_DIR}/handy/handy.h
        ${PROJECT_SOURCE_DIR}/handy/handy-imp.h
        ${PROJECT_SOURCE_DIR}/handy/http.h
        ${PROJECT_SOURCE_DIR}/handy/logging.h
        ${PROJECT_SOURCE_DIR}/handy/net.h
        ${PROJECT_SOURCE_DIR}/handy/poller.h
        ${PROJECT_SOURCE_DIR}/handy/port_posix.h
        ${PROJECT_SOURCE_DIR}/handy/slice.h
        ${PROJECT_SOURCE_DIR}/handy/stat-svr.h
        ${PROJECT_SOURCE_DIR}/handy/status.h
        ${PROJECT_SOURCE_DIR}/handy/threads.h
        ${PROJECT_SOURCE_DIR}/handy/udp.h
        ${PROJECT_SOURCE_DIR}/handy/util.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/handy)
endif(BUILD_HANDY_SHARED_LIBRARY OR BUILD_HANDY_STATIC_LIBRARY)


function(add_handy_executable EXECUTABLE_NAME EXECUTABLE_SOURCES)
    add_executable(${EXECUTABLE_NAME} ${EXECUTABLE_SOURCES})
    target_link_libraries(${EXECUTABLE_NAME} handy_s)
    target_include_directories(${EXECUTABLE_NAME} PUBLIC ${PROJECT_SOURCE_DIR})
    install(TARGETS ${EXECUTABLE_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
endfunction(add_handy_executable)


if(BUILD_HANDY_EXAMPLES)
    add_handy_executable(codec-cli examples/codec-cli.cc)
    add_handy_executable(codec-svr examples/codec-svr.cc)
    add_handy_executable(daemon examples/daemon.cc)
    add_handy_executable(echo examples/echo.cc)
    add_handy_executable(hsha examples/hsha.cc)
    add_handy_executable(http-hello examples/http-hello.cc)
    add_handy_executable(idle-close examples/idle-close.cc)
    add_handy_executable(reconnect examples/reconnect.cc)
    add_handy_executable(safe-close examples/safe-close.cc)
    add_handy_executable(stat examples/stat.cc)
    add_handy_executable(timer examples/timer.cc)
    add_handy_executable(udp-cli examples/udp-cli.cc)
    add_handy_executable(udp-hsha examples/udp-hsha.cc)
    add_handy_executable(udp-svr examples/udp-svr.cc)
    add_handy_executable(write-on-empty examples/write-on-empty.cc)
    add_handy_executable(10m-cli 10m/10m-cli.cc)
    add_handy_executable(10m-svr 10m/10m-svr.cc)

    if(CMAKE_HOST_APPLE)
        add_handy_executable(kqueue raw-examples/kqueue.cc)
    endif(CMAKE_HOST_APPLE)
endif(BUILD_HANDY_EXAMPLES)
