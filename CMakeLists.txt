cmake_minimum_required(VERSION 3.17)
project(simple_ffmpeg_tutorial C)

set(CMAKE_C_STANDARD 11)

include_directories(/usr/local/ffmpeg/include)
link_directories(/usr/local/ffmpeg/lib)

add_executable(simple_ffmpeg_tutorial
        main.c)

target_link_libraries(simple_ffmpeg_tutorial avformat avutil avdevice)