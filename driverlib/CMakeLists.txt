# driverlib/CMakeLists.txt

# src 폴더의 모든 소스파일을 컴파일하고, include 폴더를 헤더 경로로 등록합니다.
idf_component_register(SRC_DIRS "src"
                       INCLUDE_DIRS "include")