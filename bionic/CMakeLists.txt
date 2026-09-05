cmake_minimum_required(VERSION 3.14)
project(bionic LANGUAGES C CXX)

# Integrated Bionic — owned, not vendor-tracked
# This directory is a full AOSP Bionic checkout (pruned of benchmarks/tests)
# Real build of bionic/libm etc. requires Android build system; for now we
# expose headers and version scripts for bionic-libm to reference.
# Future: add bionic_m_real SHARED here that compiles msun with Bionic headers
# and proper -D__BIONIC__ etc., via custom toolchain.

message(STATUS "bionic: integrated AOSP Bionic at ${CMAKE_CURRENT_SOURCE_DIR} (headers + maps only, libm build via bionic-libm)")
