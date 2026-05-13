# YoloUltralytics.cmake
# 通用的YoloUltralytics模块构建脚本
# 支持目标检测(TargetDetection)、关键点检测(KeyPoint)和分类(Classification)
# 支持同时构建多个模块

# 定义一个函数来创建YoloUltralytics模块
function(add_yolo_ultralytics_module MODULE_TYPE MODULE_NAME)
    # 参数验证
    if(NOT MODULE_TYPE STREQUAL "TargetDetection"
        AND NOT MODULE_TYPE STREQUAL "KeyPoint"
        AND NOT MODULE_TYPE STREQUAL "Classification")
        message(FATAL_ERROR "MODULE_TYPE 必须为 TargetDetection、KeyPoint 或 Classification")
    endif()

    set(INFERENCE_NAME "${MODULE_NAME}")

    # 根据模块类型设置目录路径
    if(MODULE_TYPE STREQUAL "TargetDetection")
        set(MODULE_DIR "${AI_SHARE_PATH}/AiModules/Inference/Hisilicon/TargetDetection/${MODULE_NAME}")
    elseif(MODULE_TYPE STREQUAL "KeyPoint")
        set(MODULE_DIR "${AI_SHARE_PATH}/AiModules/Inference/Hisilicon/KeyPoint/${MODULE_NAME}")
    elseif(MODULE_TYPE STREQUAL "Classification")
        set(MODULE_DIR "${AI_SHARE_PATH}/AiModules/Inference/Hisilicon/Classification/${MODULE_NAME}")
    endif()

    # 检查模块目录是否存在
    if(NOT EXISTS ${MODULE_DIR})
        message(FATAL_ERROR "模块目录不存在: ${MODULE_DIR}")
    endif()

    # 创建库
    add_library(${INFERENCE_NAME} INTERFACE)

    # 清除变量（使用局部变量）
    set(MODULE_SOURCES_LOCAL)

    # 文件路径配置
    set (SRC_PATH
        # 通用依赖
        ${MODULE_DIR}/../../../
        # ${MODULE_DIR}/../../../PostProcess/
        # 芯片推理依赖
        ${MODULE_DIR}/../../
        ${MODULE_DIR}/../../Common/ModelOpt/
    )

    # 仅检测与关键点模块依赖后处理目录
    if(NOT MODULE_TYPE STREQUAL "Classification")
        list(APPEND SRC_PATH ${MODULE_DIR}/../../../PostProcess/)
    endif()

    # 添加源文件目录
    foreach(item ${SRC_PATH})
        target_include_directories(${INFERENCE_NAME} INTERFACE ${item})
        aux_source_directory(${item} MODULE_SOURCES_LOCAL)
    endforeach()

    # 过滤规则 (防止包含demo文件)
    set(GREP_TEXT "demo|Demo")

    # 获取本程序子目录
    execute_process(
        COMMAND find ${MODULE_DIR}/ -type d 
        COMMAND grep -vE ${GREP_TEXT} # 过滤目录
        OUTPUT_VARIABLE dirs_list
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )

    # 处理目录列表
    string(REPLACE "\n" ";" dirs_list ${dirs_list})

    # 添加所有源文件与头文件
    foreach(item ${dirs_list})
        if(EXISTS ${item})
            target_include_directories(${INFERENCE_NAME} INTERFACE ${item})
            aux_source_directory (${item} MODULE_SOURCES_LOCAL)
        endif()
    endforeach() 

    # 设置的源文件
    target_sources(${INFERENCE_NAME} INTERFACE ${MODULE_SOURCES_LOCAL})

    # 链接库路径
    target_link_libraries(${INFERENCE_NAME} INTERFACE 
        securec
    )

    # 输出信息
    message("==========> [${PROJECT_NAME}] 使用[${INFERENCE_NAME}]模块 (${MODULE_TYPE}) <===========")

    # 可选：输出调试信息
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(STATUS "模块类型: ${MODULE_TYPE}")
        message(STATUS "模块名称: ${MODULE_NAME}")
        message(STATUS "模块目录: ${MODULE_DIR}")
        list(LENGTH MODULE_SOURCES_LOCAL source_count)
        message(STATUS "源文件数量: ${source_count}")
    endif()
endfunction()

# --- 便捷函数：添加目标检测模块 ---
function(add_yolo_target_detection MODULE_NAME)
    add_yolo_ultralytics_module("TargetDetection" ${MODULE_NAME})
endfunction()

# --- 便捷函数：添加关键点检测模块 ---
function(add_yolo_keypoint MODULE_NAME)
    add_yolo_ultralytics_module("KeyPoint" ${MODULE_NAME})
endfunction()

# --- [新增] 便捷函数：添加图像分类模块 ---
function(add_yolo_classification MODULE_NAME)
    add_yolo_ultralytics_module("Classification" ${MODULE_NAME})
endfunction()

# 预定义的模块添加函数
function(add_yolo_ultralytics_rpn)
    add_yolo_target_detection("YoloUltralytics_rpn")
endfunction()

function(add_yolo_ultralytics_point_rpn)
    add_yolo_keypoint("YoloUltralyticsPoint_rpn")
endfunction()

function(add_image_feature)
    add_yolo_ultralytics_module("Classification" "ImageFeature")
endfunction()

# 批量添加所有常用模块的函数
function(add_all_yolo_modules)
    add_yolo_ultralytics_rpn()
    add_yolo_ultralytics_point_rpn()
    add_image_feature()
    message("==========> 已添加所有YoloUltralytics模块 <===========")
endfunction()
