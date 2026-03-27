cmake_minimum_required(VERSION 3.16)

# 👇 [추가된 핵심 코드] 상위 폴더의 driverlib을 추가 라이브러리로 강제 인식시킵니다.
set(EXTRA_COMPONENT_DIRS "../../driverlib")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(Example_GPIO_Interrupt)