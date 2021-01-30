cmake_minimum_required(VERSION 3.15)
project(c20)

set(CMAKE_CXX_STANDARD 20)

SET(CMAKE_C_COMPILER "/usr/local/bin/gcc-10")
SET(CMAKE_CXX_COMPILER "/usr/local/bin/g++-10")

include_directories(
        "/Users/weideng/projects/github.com/ffmpeg"
)

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ")

link_directories(/Users/weideng/projects/github.com/ffmpeg/lib)

#set(CMAKE_CXX_FLAGS "-std=c++2a -DASIO_STAND_ALONE -fcoroutines-ts -Iinclude -stdlib=libc++ -std=c++17")

add_executable(c20 main.cpp thread.cpp function.cpp tools.cpp tffmpeg.cpp tmp.cpp tcp.cpp)

TARGET_LINK_LIBRARIES(c20 avformat avcodec avdevice avfilter avutil swresample swscale)