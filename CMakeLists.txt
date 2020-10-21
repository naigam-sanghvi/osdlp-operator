cmake_minimum_required(VERSION 3.10)

# set the project name and version
project(osdlp-operator LANGUAGES C CXX VERSION 1.0)

# Make sure our local CMake Modules path comes first
list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake/Modules)

SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-pthread")

set(CMAKE_BUILD_TYPE "RelwithDebInfo")

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#configure_file(
#    ${CMAKE_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in
#    ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
#@ONLY)

find_package(Config++ REQUIRED)

add_subdirectory(lib)
add_subdirectory(src)

