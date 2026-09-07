# detection.cmake
# 统一检测层（每通道单 Worker 串行执行已注册模型）
#
# 编译条件：
# - IPC_CAP_UNIFIED_DETECTION_MD=1：编译移动侦测模型适配器 + 移动侦测逻辑
# - IPC_CAP_UNIFIED_DETECTION_OD=1：编译遮挡侦测模型适配器 + 遮挡侦测逻辑
# - IPC_CAP_AI_EXHIBITION_PEOPLE_FLOW=1：编译展馆人流统计模型（推理+跟踪+转换+分发）
# - IPC_CAP_AI_PEOPLE_DENSITY_PIPELINE=1：追加展馆人员密度输出执行器（依赖展馆模型）
# - 引擎/帧准备器在任一宏开启时编译
# - 全 0（LEGACY）时不编译检测层任何源文件，编译产物不引入检测层符号依赖

if(IPC_CAP_UNIFIED_DETECTION_MD EQUAL 1 OR IPC_CAP_UNIFIED_DETECTION_OD EQUAL 1
   OR IPC_CAP_AI_EXHIBITION_PEOPLE_FLOW EQUAL 1)
    set(DETECTION_DIR ${CMAKE_CURRENT_LIST_DIR})

    # 头文件目录
    include_directories(
        ${DETECTION_DIR}
    )

    # 检测层核心源文件（引擎 + 帧准备器合并为 detection_engine）
    set(DETECTION_SOURCES
        ${DETECTION_DIR}/detection_engine.cpp
    )

    # 移动侦测模型（svp_md）
    if(IPC_CAP_UNIFIED_DETECTION_MD EQUAL 1)
        list(APPEND DETECTION_SOURCES
            ${DETECTION_DIR}/motion_detect/motion_detect_logic.cpp
            ${DETECTION_DIR}/motion_detect/motion_detect_model.cpp
        )
    endif()

    # 遮挡侦测模型（svp_od）
    if(IPC_CAP_UNIFIED_DETECTION_OD EQUAL 1)
        list(APPEND DETECTION_SOURCES
            ${DETECTION_DIR}/hide_detect/hide_detect_logic.cpp
            ${DETECTION_DIR}/hide_detect/hide_detect_model.cpp
        )
    endif()

    # 展馆人流统计模型（YOLOV8n-exhibition）
    # 推理引擎抽象层：i_inference_engine.hpp（接口）+ yolo/hiai_detect 引擎实现
    #   - yolo_inference_engine：包装 YoloUltralytics（自研模型）
    #   - hiai_detect_inference_engine：包装 HiAiDetect_S（海思引擎，可加载 HVF/宠物等模型）
    # 依赖共享 cIouTracker（share/ai_share/AiModules/Inference/CPU/Tracker/IouTrack）：
    # 显式加入 IouTracker.cpp 源码与 include 路径（含 Eigen 头文件所在 CPU/Common 目录），
    # 不修改 share/ai_share 的 cmake 构建链
    if(IPC_CAP_AI_EXHIBITION_PEOPLE_FLOW EQUAL 1)
        list(APPEND DETECTION_SOURCES
            ${DETECTION_DIR}/inference/yolo_inference_engine.cpp
            ${DETECTION_DIR}/inference/hiai_detect_inference_engine.cpp
            ${DETECTION_DIR}/exhibition_detect/exhibition_detect_logic.cpp
            ${DETECTION_DIR}/exhibition_detect/exhibition_detect_model.cpp
            ${DETECTION_DIR}/exhibition_detect/exhibition_result_converter.cpp
            ${AI_SHARE_PATH}/AiModules/Inference/CPU/Tracker/IouTrack/IouTracker.cpp
        )
        include_directories(
            ${DETECTION_DIR}/inference
            ${AI_SHARE_PATH}/AiModules/Inference/CPU/Tracker/IouTrack
            ${AI_SHARE_PATH}/AiModules/Inference/CPU/Common
        )

        # 人员密度新框架输出执行器（head 跟踪结果 -> 三级报警 + 周期上报）
        if(IPC_CAP_AI_PEOPLE_DENSITY_PIPELINE EQUAL 1)
            list(APPEND DETECTION_SOURCES
                ${DETECTION_DIR}/exhibition_detect/exhibition_density_output_executor.cpp
            )
        endif()
    endif()

    list(APPEND SRC_LIST ${DETECTION_SOURCES})
endif()
