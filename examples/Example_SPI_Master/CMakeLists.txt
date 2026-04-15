# templates/default_project/CMakeLists.txt

cmake_minimum_required(VERSION 3.16)

# 👇 [추가된 핵심 코드] 상위 폴더의 driverlib을 추가 라이브러리로 강제 인식시킵니다.
set(EXTRA_COMPONENT_DIRS "../../driverlib")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
idf_build_set_property(MINIMAL_BUILD ON) # [선택적] 빌드 최적화: 필요한 컴포넌트만 빌드하도록 설정 (예시에서는 최소 빌드로 설정)
project(Example_SPI_Master)