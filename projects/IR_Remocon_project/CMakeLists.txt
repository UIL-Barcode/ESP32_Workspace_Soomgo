# templates/default_project/CMakeLists.txt

cmake_minimum_required(VERSION 3.16)

# 👇 [추가된 핵심 코드] 상위 폴더의 driverlib을 추가 라이브러리로 강제 인식시킵니다.
set(EXTRA_COMPONENT_DIRS "../../driverlib")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
idf_build_set_property(MINIMAL_BUILD ON)
project(IR_Remocon_project)