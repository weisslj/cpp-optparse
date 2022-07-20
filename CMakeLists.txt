cmake_minimum_required(VERSION 3.18)

project(cpp-optparse)

set(CMAKE_CXX_STANDARD 11)

if ((CMAKE_CXX_COMPILER_ID MATCHES GNU) OR (CMAKE_CXX_COMPILER_ID MATCHES Clang))
  set(CMAKE_CXX_FLAGS "-O3 -g -Wall -Wextra -Werror")
# Not sure if these flags still apply to IntelLLVM
elseif (CMAKE_CXX_COMPILER_ID MATCHES Intel)
  set(CMAKE_CXX_FLAGS "-O3 -ipo -g -Wall -wd981 -wd383 -wd2259 -Werror")
endif()

add_library(cpp-optparse OBJECT OptionParser.cpp OptionParser.h)

add_executable(testprog testprog.cpp)

target_link_libraries(testprog cpp-optparse)

enable_testing()
add_test(NAME Test COMMAND testprog "${CMAKE_SOURCE_DIR}/test.sh")
