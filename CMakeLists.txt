cmake_minimum_required(VERSION 3.3)
project(stat)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES application.cpp)
set(SOURCE_FILES Cellar.h Cellar.cpp)
set(SOURCE_FILES common.h common.c)
set(SOURCE_FILES OledDisplay.h OledDisplay.cpp)
set(SOURCE_FILES RHT03.h RHT03.cpp)
set(SOURCE_FILES HIH6130.h HIH6130.cpp)
set(SOURCE_FILES ButtonInterrupt.h ButtonInterrupt.cpp)
set(SOURCE_FILES oled_font.h oled_font.cpp)
set(SOURCE_FILES font_lcd6x8.h font_lcd11x16.h)
add_executable(stat ${SOURCE_FILES})

include_directories("/Users/jsokel/particle/firmware/inc")