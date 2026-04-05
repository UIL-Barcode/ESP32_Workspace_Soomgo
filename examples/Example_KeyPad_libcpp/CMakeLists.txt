# examples/Example_KeyPad_libcpp/CMakeLists.txt

cmake_minimum_required(VERSION 3.16)

get_filename_component(WORKSPACE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../" ABSOLUTE)

set(EXTRA_COMPONENT_DIRS "${WORKSPACE_ROOT}/driverlib")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(Example_KeyPad_libcpp)