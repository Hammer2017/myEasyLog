cmake_minimum_required(VERSION 3.29)
project(easyLogging__)

set(CMAKE_CXX_STANDARD 14)

#设置debug模式
set(CMAKE_BUILD_TYPE Debug)

# 启用调试信息
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")

add_executable(easyLogging__ main.cpp easy_log/easylogging++.cc
        LogHelper.cpp
        LogHelper.h
)

target_include_directories(easyLogging__ PUBLIC easy_log)
target_compile_definitions(easyLogging__ PUBLIC ELPP_FEATURE_CRASH_LOG HAVE_EXECINFO)
# Link pthread library
target_link_libraries(easyLogging__ pthread)