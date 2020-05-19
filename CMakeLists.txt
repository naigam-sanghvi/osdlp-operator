cmake_minimum_required(VERSION 3.10)

# set the project name and version
project(osdlp-operator LANGUAGES C CXX VERSION 1.0)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#configure_file(
#    ${CMAKE_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in
#    ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
#@ONLY)

add_subdirectory(lib)
add_subdirectory(src)

