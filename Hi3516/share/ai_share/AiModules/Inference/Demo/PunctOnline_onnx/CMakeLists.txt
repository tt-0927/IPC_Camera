# 版本限定
cmake_minimum_required(VERSION 3.12)

# 设置程序名称
project("CTTransformerOnlinePunct_Demo")

# CPU_ARM_64
set(CROSS_PREFIX "aarch64-buildroot-linux-gnu-")
set(Framework cpu_arm_64)
# CPU_X86_64 或 GPU_X86_64
# set(CROSS_PREFIX "")
# set(Framework gpu_x84_64)

# 编译链设置
string(STRIP "${CROSS_PREFIX}" CROSS_PREFIX)
# 根据前缀自动设置编译工具
set(CMAKE_C_COMPILER    "${CROSS_PREFIX}gcc")
set(CMAKE_CXX_COMPILER  "${CROSS_PREFIX}g++")
set(CMAKE_AR            "${CROSS_PREFIX}ar")
set(CMAKE_RANLIB        "${CROSS_PREFIX}ranlib")
set(CMAKE_LINKER        "${CROSS_PREFIX}ld")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g") 

# 推理依赖
include(${CMAKE_CURRENT_LIST_DIR}/../../../../../ai_model/${Framework}/ai/ai.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../Onnx/ASR/CTTransformerOnlinePunct/CTTransformerOnlinePunct.cmake)

# 文件  
set (SRC_PATH
    # 后处理依赖
    ${CMAKE_CURRENT_LIST_DIR}/../../Common/json
)
foreach(item ${SRC_PATH})
    target_include_directories(${INFERENCE_NAME} INTERFACE ${item})
    aux_source_directory(${item} MODULE_SOURCES)
endforeach()

SET(LIBS
    CTTransformerOnlinePunct
)

# 设置生成文件存放路径
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../output/bin)

# 编译
add_executable(${PROJECT_NAME} ${OS_SHARE_SRC_LIST} ${MODULE_SOURCES} main.cpp)

# 链接库路径
target_link_libraries(${PROJECT_NAME}
    ${LIBS}
)


