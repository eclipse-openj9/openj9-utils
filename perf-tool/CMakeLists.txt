cmake_minimum_required(VERSION 3.18)

project(perf-agent)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(BUILD_SHARED_LIBS ON)

include_directories(include)

include_directories($ENV{JAVA_HOME}/include)

include_directories($ENV{JAVA_HOME}/include/linux)

file(GLOB SOURCES "src/*.cpp")

add_library(agent SHARED ${SOURCES})

