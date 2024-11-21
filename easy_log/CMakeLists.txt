cmake_minimum_required(VERSION 3.29)
project(easyLogging_shared)

set(CMAKE_CXX_STANDARD 14)

# 设置库的输出目录
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 编译动态库
add_library(loggingcpp SHARED easylogging++.cc)


# 添加库的头文件目录
target_include_directories(loggingcpp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(loggingcpp PUBLIC ELPP_FEATURE_CRASH_LOG HAVE_EXECINFO)


#编译完成后拷贝生成的库到指定目录
add_custom_command(TARGET loggingcpp POST_BUILD
        COMMAND cp ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libloggingcpp.so  ${CMAKE_CURRENT_SOURCE_DIR}/../lib)

# 设置库版本号 (可选)
#set_target_properties(easyloggingpp PROPERTIES
#        VERSION 9.97.1
#        SOVERSION 1
#)