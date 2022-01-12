cmake_minimum_required(VERSION 3.21)
project(pilk VERSION 1.0.0)

# 设置编译类型，也可在CLION设置中设置
# set(CMAKE_BUILD_TYPE Release)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

# Python相关
set(PYTHON_HOME "C:/Users/foyou/AppData/Local/Programs/Python/Python_d310")
include_directories(${PYTHON_HOME}/include)
link_directories(${PYTHON_HOME}/libs)

# silk
include_directories(src/interface)
include_directories(src/SKP_SILK_SRC)

add_library(pilk SHARED src/pilkmodule.c)

add_subdirectory(src/SKP_SILK_SRC)

target_link_libraries(pilk PUBLIC SKP_Silk_SDK)

# 设置动态库属性
SET_TARGET_PROPERTIES(
        pilk PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/dev
        PREFIX ""
        OUTPUT_NAME "_pilk_d"
        SUFFIX ".pyd"
)

if (CMAKE_BUILD_TYPE MATCHES Debug)
    message(" ")
    message("CMAKE IN DEBUG MODE")
    message(" ")
elseif (CMAKE_BUILD_TYPE MATCHES Release)
    message(" ")
    message("CMAKE IN RELEASE MODE")
    message(" ")
endif ()