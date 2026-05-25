# templates/default_project/main/CMakeLists.txt
set(req "nvs_flash"
        "bt"
    )

# 1. main 폴더 안에 있는 모든 .c 파일을 자동으로 찾아서 srcs 변수에 담습니다.
file(GLOB_RECURSE srcs "*.c")

# 2. 찾은 모든 소스 파일과, 현재 폴더(.)의 헤더 파일들을 컴포넌트로 등록합니다.
idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES ${req})