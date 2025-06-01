cmake_minimum_required(VERSION 3.15...3.27)

project(
  ${SKBUILD_PROJECT_NAME}
  VERSION ${SKBUILD_PROJECT_VERSION}
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MES_UTILS
add_library(mes_utils STATIC src/mes_utils.cpp)
set_target_properties(mes_utils PROPERTIES POSITION_INDEPENDENT_CODE ON)

# BASIC_MES
add_library(mes STATIC src/mes.cpp)
set_target_properties(mes PROPERTIES POSITION_INDEPENDENT_CODE ON)

find_package(Python REQUIRED COMPONENTS Interpreter Development.Module)
find_package(pybind11 CONFIG REQUIRED)

python_add_library(muoblpbindings MODULE src/binder.cpp WITH_SOABI)
target_link_libraries(muoblpbindings PRIVATE pybind11::headers)
target_link_libraries(muoblpbindings PRIVATE mes_utils)
target_link_libraries(muoblpbindings PRIVATE mes)

target_compile_definitions(muoblpbindings PRIVATE VERSION_INFO=${PROJECT_VERSION})

install(TARGETS muoblpbindings DESTINATION muoblpbindings)
