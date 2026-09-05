cmake_minimum_required(VERSION  3.0 ... 4.0)

set(CMAKE_CXX_STANDARD 11)


project(logger LANGUAGES CXX)

add_library(logger include/logger/log.h src/log.cpp)
target_include_directories(logger PUBLIC include/)