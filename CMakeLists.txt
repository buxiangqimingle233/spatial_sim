cmake_minimum_required (VERSION 3.5)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

# set(PYBIND11_PYTHON_VERSION 3.8)

add_subdirectory(pybind11)
add_subdirectory(src)

# add_compile_options(VERSION_INFO=${EXAMPLE_VERSION_INFO})
