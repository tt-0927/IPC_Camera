# detection.cmake
# 统一检测层（每通道单 Worker 串行执行已注册模型）
#
# 编译条件（两级门控）：
# - IPC_CAP_UNIFIED_DETECTION=1：编译检测层全部源文件——引擎/帧准备器 + 已迁移模型
#   （移动侦测 MD、遮挡侦测 OD、人脸抓拍 FACE）+ 通用跟踪模块
# - IPC_CAP_AI_EXHIBITION_PEOPLE_FLOW=1：在总开关基础上追加展馆人流统计模型
#   （推理+跟踪+转换+分发），ipc.cmake 校验其依赖总开关=1
# - IPC_CAP_AI_PEOPLE_DENSITY_PIPELINE=1：再追加展馆人员密度输出执行器（依赖展馆模型）
# - 全 0（LEGACY）时不编译检测层任何源文件，Legacy algorithm/ 链路原样编译运行
# - 未迁移算法（人流/密度/事件管线等）走 Legacy 链路，迁移完成后随总开关纳入
#
# 人脸业务组件归属（总开关=1 时生效）：
# - 抓拍处理器（capture/）与推理 Worker（worker/）为本层独立实现的同名超集类，
#   Legacy 侧同名源在此剔除（保证全编译单元单一定义），头文件路径以 BEFORE
#   前置接管全部编译单元；名单库临时实例等 Legacy 调用方无感知复用本实现
# - 名单库服务（library/）承载上传图入库：统一检测层画像的 algo_stream_deal
#   改接本服务；总开关=0 的 Legacy 画像不编译本层，仍走 Legacy 临时实例路径
# - 特征比对处理器仍编译 Legacy 源（未被优化线改动，两条链路共用）

if(IPC_CAP_UNIFIED_DETECTION EQUAL 1)
    set(DETECTION_DIR ${CMAKE_CURRENT_LIST_DIR})

    # 头文件目录
    include_directories(
        ${DETECTION_DIR}
    )

    # 人脸 capture/worker 头文件前置接管（排在 Legacy face_detect 目录扫描之前，
    # 同名头 face_capture_processor.hpp / face_detect_worker.hpp 以本层为准）
    include_directories(BEFORE
        ${DETECTION_DIR}/face_detect/capture
        ${DETECTION_DIR}/face_detect/worker
    )
    # 人脸链路公共头（face_chain_types.hpp / bestshot_hub.hpp）
    include_directories(
        ${DETECTION_DIR}/face_detect
    )

    # 检测层源文件：引擎（含帧准备器）+ 已迁移模型（MD/OD/FACE）+ 通用跟踪模块
    # 通用跟踪 + 共享 cIouTracker 一并编入（face_detect 与展馆共用，只编一次）
    # compare 适配层与名单库服务随 CAP_AI_FACE_COMPARE 宏整体空编译（无比对画像零符号开销）
    set(DETECTION_SOURCES
        ${DETECTION_DIR}/detection_engine.cpp
        ${DETECTION_DIR}/motion_detect/motion_detect_logic.cpp
        ${DETECTION_DIR}/motion_detect/motion_detect_model.cpp
        ${DETECTION_DIR}/hide_detect/hide_detect_logic.cpp
        ${DETECTION_DIR}/hide_detect/hide_detect_model.cpp
        ${DETECTION_DIR}/face_detect/face_detect_model.cpp
        ${DETECTION_DIR}/face_detect/bestshot_hub.cpp
        ${DETECTION_DIR}/face_detect/capture/face_capture_processor.cpp
        ${DETECTION_DIR}/face_detect/capture/ive_sharpness_adapter.cpp
        ${DETECTION_DIR}/face_detect/compare/face_compare_processor.cpp
        ${DETECTION_DIR}/face_detect/library/face_library_service.cpp
        ${DETECTION_DIR}/face_detect/worker/face_detect_worker.cpp
        ${DETECTION_DIR}/tracker/detection_tracker.cpp
        ${AI_SHARE_PATH}/AiModules/Inference/CPU/Tracker/IouTrack/IouTracker.cpp
    )
    include_directories(
        ${DETECTION_DIR}/tracker
        ${AI_SHARE_PATH}/AiModules/Inference/CPU/Tracker/IouTrack
        ${AI_SHARE_PATH}/AiModules/Inference/CPU/Common
    )

    # 剔除 Legacy 侧同名源：抓拍处理器与推理 Worker 已由本层超集实现接管，
    # 同名类只允许一份定义；Legacy 头文件经 BEFORE 前置路径被本层头遮蔽。
    # 总开关=0 时本文件整体不生效，Legacy face_detect 按导入基线原样编译。
    list(REMOVE_ITEM SOURCE_LIST
        ${FACE_DETECT_ROOT}/internal/processors/capture/face_capture_processor.cpp
        ${FACE_DETECT_ROOT}/internal/processors/face_detect_worker/face_detect_worker.cpp
    )

    # 展馆人流统计模型（YOLOV8n-exhibition，独立二级开关）
    # 推理引擎抽象层：i_inference_engine.hpp（接口）+ yolo/hiai_detect 引擎实现
    #   - yolo_inference_engine：包装 YoloUltralytics（自研模型）
    #   - hiai_detect_inference_engine：包装 HiAiDetect_S（海思引擎，可加载 HVF/宠物等模型）
    if(IPC_CAP_AI_EXHIBITION_PEOPLE_FLOW EQUAL 1)
        list(APPEND DETECTION_SOURCES
            ${DETECTION_DIR}/inference/yolo_inference_engine.cpp
            ${DETECTION_DIR}/inference/hiai_detect_inference_engine.cpp
            ${DETECTION_DIR}/exhibition_detect/exhibition_detect_logic.cpp
            ${DETECTION_DIR}/exhibition_detect/exhibition_detect_model.cpp
            ${DETECTION_DIR}/exhibition_detect/exhibition_result_converter.cpp
        )
        include_directories(
            ${DETECTION_DIR}/inference
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
