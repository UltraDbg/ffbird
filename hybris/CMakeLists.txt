cmake_minimum_required(VERSION 3.14)
project(hybris LANGUAGES C CXX)

# Integrated cleaned libhybris — owned
# This is a pruned libhybris (common + egl) from https://github.com/libhybris/libhybris at 7079712
# Real build requires hybris common hooks/wrappers with Android headers
# For now, hybris-bridge uses host dlopen stub; this CMake just exposes the source tree.

message(STATUS "hybris: integrated cleaned libhybris at ${CMAKE_CURRENT_SOURCE_DIR}")
