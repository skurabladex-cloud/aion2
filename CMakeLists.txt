
 cmake_minimum_required(VERSION 3.20)
project(ailon2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------- OpenCV ----------------
set(OpenCV_DIR "D:/opencv/build")
find_package(OpenCV REQUIRED)

# ---------------- Interception ----------------
# 改成你实际解压路径（包含 command line installer / library / samples 的那个目录）
set(INTERCEPTION_ROOT "D:/鼠标调用/Interception")

# 头文件在: ${INTERCEPTION_ROOT}/library/interception.h
set(INTERCEPTION_INCLUDE_DIR "${INTERCEPTION_ROOT}/library")

# 根据你工程位数选择 x64/x86
set(INTERCEPTION_LIB_DIR "${INTERCEPTION_ROOT}/library/x64")
set(INTERCEPTION_DLL     "${INTERCEPTION_LIB_DIR}/interception.dll")

# ---------------- Target ----------------
add_executable(ailon2
        core/main.cpp
        core/common.cpp
        core/core.cpp
        input/GameWindowCapturer.cpp
        tree/bt_tree.cpp
)

# 项目根目录（用于包含 interception/、input/、core/、tree/ 等）
target_include_directories(ailon2 PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# json 单头
target_include_directories(ailon2 PRIVATE
        "D:/json/json-develop/single_include"
)

# OpenCV
target_include_directories(ailon2 PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(ailon2 PRIVATE ${OpenCV_LIBS})

# Interception include + link
target_include_directories(ailon2 PRIVATE ${INTERCEPTION_INCLUDE_DIR})
target_link_directories(ailon2 PRIVATE ${INTERCEPTION_LIB_DIR})
target_link_libraries(ailon2 PRIVATE interception)

# 把 interception.dll 复制到 exe 同目录（运行时必须）
add_custom_command(TARGET ailon2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${INTERCEPTION_DLL}"
        "$<TARGET_FILE_DIR:ailon2>"
)

if (MSVC)
    target_compile_options(ailon2 PRIVATE /utf-8)
    target_compile_definitions(ailon2 PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

message(STATUS "OpenCV_DIR=${OpenCV_DIR}")
message(STATUS "OpenCV_VERSION=${OpenCV_VERSION}")
message(STATUS "OpenCV_INCLUDE_DIRS=${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV_LIBS=${OpenCV_LIBS}")

message(STATUS "INTERCEPTION_ROOT=${INTERCEPTION_ROOT}")
message(STATUS "INTERCEPTION_INCLUDE_DIR=${INTERCEPTION_INCLUDE_DIR}")
message(STATUS "INTERCEPTION_LIB_DIR=${INTERCEPTION_LIB_DIR}")
message(STATUS "INTERCEPTION_DLL=${INTERCEPTION_DLL}")
