cmake_minimum_required(VERSION 3.10)

project(HttpFaceServerDemo C)

# ===================== 编译标准 =====================
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME} main.c)

if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        _WIN32_WINNT=0x0A00
        WINVER=0x0A00
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
endif()

set(LIBS "")
if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(APPEND LIBS
        ws2_32
    )
else()
    list(APPEND LIBS
        pthread
    )
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBS})

message(STATUS "项目名称: ${PROJECT_NAME}")
message(STATUS "最终链接库: ${LIBS}")
