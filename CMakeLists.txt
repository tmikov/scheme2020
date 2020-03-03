cmake_minimum_required(VERSION 3.15)
project(Scheme2020)

include(CMakePrintHelpers)

# This needs LLVM_ROOT to be set.
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
include(S2020)
include(Lit)

add_subdirectory(cxx)
