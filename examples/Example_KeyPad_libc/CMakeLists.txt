# examples/Example_KeyPad_libc/CMakeLists.txt

cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS "../../driverlib")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(Example_KeyPad)