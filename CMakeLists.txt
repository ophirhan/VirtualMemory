cmake_minimum_required(VERSION 3.16)
project(VirtualMemory)

set(CMAKE_CXX_STANDARD 11)

add_library(VirtualMemory VirtualMemory.cpp VirtualMemory.h PhysicalMemory.h PhysicalMemory.cpp)
add_executable(simple SimpleTest.cpp)
target_link_libraries(simple PUBLIC VirtualMemory)