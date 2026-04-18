# driverlib/CMakeLists.txt

file(GLOB_RECURSE sources "*.c" "*.cpp")

# src 폴더의 모든 소스파일을 컴파일하고, include 폴더를 헤더 경로로 등록합니다.
idf_component_register(SRCS ${sources}
			INCLUDE_DIRS "."
			PRIV_REQUIRES driver)
                       