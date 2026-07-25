# 设备能力画像（业务仓库唯一入口）
# 说明：
# - DEVICE_PROFILE_*_DEFINES: 统一下发给业务/共享仓的编译宏
# - DEVICE_PROFILE_*_CMAKE_VARS: 供 CMakeLists 使用的能力变量（key=value）
#
# 命名规范：
# - CAP_*: Capability（能力）编译宏，供 C/C++ 代码使用（#if CAP_xxx）
# - IPC_CAP_*: CMake 能力变量，供 CMakeLists 使用（if(IPC_CAP_xxx)）
# - 取值统一为 0/1（1=支持/开启，0=不支持/关闭）
# - 命名建议为正向语义：CAP_<模块>_<能力>
#
# 宏语义与主要使用位置（便于检索）：
# - CAP_ALARM_IO: 报警输入/输出能力
#   使用: rv1126b_ipc/include/share_define.h,
#         share/ipc_share/control/task/sub_task/event_task.cpp,
#         share/ipc_share/control/business/event/event_alarm.cpp
# - CAP_AUDIO_INPUT_LINEIN: LineIn 输入能力
#   使用: share/ipc_share/common/define/device_capability.h
# - CAP_AUDIO_OUTPUT_LINEOUT: LineOut 输出能力
#   使用: share/ipc_share/common/define/device_capability.h
# - CAP_AUDIO_FMT_AAC/G711U/G711A/PCM/G726: 音频编码格式能力
#   使用: share/ipc_share/control/business/av/av_configure.cpp
# - CAP_MOTION_REGION_GRID: 移动侦测网格区域能力
#   使用: share/ipc_share/common/define/alarm_define.h
# - CAP_VIDEO_MAX_2_5K: 视频最大分辨率到 2.5K
#   使用: share/ipc_share/common/config_compat/video_config_compat.h
# - CAP_VIDEO_MAX_4K: 视频最大分辨率到 4K
#   使用: share/ipc_share/common/config_compat/video_config_compat.h
# - CAP_SMART_EVENT_PERF_LIMIT: 智能事件性能限制策略
#   使用: share/ipc_share/control/task/sub_task/system_task.cpp
# - CAP_ISP_IR_SWITCH: ISP 红外切换能力
#   使用: share/ipc_share/control/business/isp/isp_manage.cpp
# - CAP_GPIO_LAYOUT_852_SERIES: 852 系列 GPIO 布局
#   使用: share/ipc_share/control/business/system/gpioCtrl/gpio_ctrl.h
# - CAP_GPIO_LAYOUT_RV1126: RV1126 系列 GPIO 布局
#   使用: share/ipc_share/control/business/system/gpioCtrl/gpio_ctrl.h
# - CAP_LIGHT_WHITE_ONLY: 仅白光灯能力
#   使用: share/ipc_share/control/business/system/pwmCtrl/pwm_ctrl.h,
#         share/ipc_share/control/business/system/pwmCtrl/light_manager.cpp
# - CAP_PWM_NEED_POLARITY: PWM 需要极性配置
#   使用: share/ipc_share/hardware/pwm_utils/pwm_utils.cpp
# - CAP_EVENT_AUDIO_PLAYBACK_V2: 事件音频播放 V2 路径
#   使用: share/ipc_share/control/business/event/event_linkage.cpp
# - CAP_AUDIO_PLAYBACK_SLEEP_HALF: 音频播放 half-sleep 策略
#   使用: share/ipc_share/control/business/event/event_linkage.cpp
# - CAP_SYSTEM_REBOOT_MUTE: 系统重启静音处理
#   使用: share/ipc_share/control/business/system/system_manage.cpp,
#         share/ipc_share/control/business/event/event_resource.cpp
# - CAP_STORAGE_MMCBLK1: 存储设备路径 mmcblk1 逻辑
#   使用: share/ipc_share/control/business/storage/storage_manage.cpp
# - CAP_STORAGE_HAS_EMMC: 内部 eMMC 始终存在（TV-3882TI），TF 卡检测需排除 eMMC 设备
#   使用: share/ipc_share/control/business/event/monitor/system_monitor.cpp
# - CAP_PROCESS_USE_PS_GREP: 进程探测使用 ps|grep
#   使用: share/ipc_share/public_app/daemon/process_manager.cpp
# - CAP_PROCESS_KILL_CROND_BEFORE_STREAM: 拉流前处理 crond
#   使用: share/ipc_share/public_app/daemon/process_manager.cpp
# - CAP_DAEMON_LOG_BY_SIZE: 守护进程按文件大小滚动日志
#   使用: share/ipc_share/public_app/daemon/main.cpp
# - CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE: record 需要额外 cam_share include
#   使用: rv1126b_ipc/ipc.cmake, share/ipc_share/public_app/record/CMakeLists.txt
# - CAP_RECORD_LINK_FDK_AAC: record 需要链接 fdk-aac
#   使用: rv1126b_ipc/ipc.cmake, share/ipc_share/public_app/record/CMakeLists.txt
# - CAP_AI_USE_SIMPLE_JSON: AI 使用 simple Json 头
#   使用: share/ai_share/AiModules/Inference/Hisilicon/CVInferenceHISI.hpp
# - CAP_EXHIBITION_OSD_PANEL: 展会版左上角 AI 汇总面板能力
#   使用: rv1126b_ipc/main_app/ai_app/common/common_process.cpp,
#         rv1126b_ipc/main_app/av/video/process/osd/osd_manage.cpp,
#         rv1126b_ipc/main_app/av/video/process/osd/overlay_draw.cpp
# - CAP_NETWORK_TELNET_SERVICE: 启动 telnet 服务能力
#   使用: share/ipc_share/control/business/network/network_manage.cpp
# - CAP_NETWORK_FTP_SERVICE: 启动 ftp 服务能力
#   使用: share/ipc_share/control/business/network/network_manage.cpp
# - IPC_CAP_EXHIBITION_OSD_PANEL: CMake 变量版展会面板能力
#   使用: rv1126b_ipc/main_app/ai_app/common/common.cmake
# - IPC_CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE: CMake 变量版 record include 能力
#   使用: rv1126b_ipc/ipc.cmake, share/ipc_share/public_app/record/CMakeLists.txt
# - IPC_CAP_RECORD_LINK_FDK_AAC: CMake 变量版 record 链接能力
#   使用: rv1126b_ipc/ipc.cmake, share/ipc_share/public_app/record/CMakeLists.txt

set(DEVICE_PROFILE_KEYS
    TV_3881T
    TV_3882TI
)

set(DEVICE_PROFILE_TV_3881T_DEVICE_TYPE "TV-3881T")
# TV-3881T 设备能力画像（RV1126 平台基线）
set(DEVICE_PROFILE_TV_3881T_DEFINES
    DEVICE_TV_3881T
    SCENE_INTELLIGENCE                    # 场景智能能力开关（历史业务宏）

    CAP_ALARM_IO=1                        # 报警IO能力（event_task.cpp / event_alarm.cpp / share_define.h）
    CAP_AUDIO_INPUT_LINEIN=1              # LineIn 输入能力（device_capability.h）
    CAP_AUDIO_OUTPUT_LINEOUT=1            # LineOut 输出能力（device_capability.h）
    CAP_AUDIO_FMT_AAC=1                   # AAC 编码能力（av_configure.cpp）
    CAP_AUDIO_FMT_G711U=1                 # G.711ulaw 编码能力（av_configure.cpp）
    CAP_AUDIO_FMT_G711A=1                 # G.711alaw 编码能力（av_configure.cpp）
    CAP_AUDIO_FMT_PCM=1                   # PCM 编码能力（av_configure.cpp）
    CAP_AUDIO_FMT_G726=0                  # G.726 编码能力（av_configure.cpp）
    CAP_MOTION_REGION_GRID=1              # 移动侦测网格能力（alarm_define.h）
    CAP_VIDEO_MAX_2_5K=0                  # 视频最大分辨率 2.5K（video_config_compat.h）
    CAP_VIDEO_MAX_4K=1                    # 视频最大分辨率 4K（video_config_compat.h）
    CAP_SMART_EVENT_PERF_LIMIT=0          # 智能事件性能限制（system_task.cpp）
    CAP_ISP_IR_SWITCH=0                   # ISP 红外切换（isp_manage.cpp）
    CAP_GPIO_LAYOUT_852_SERIES=0          # 852 系列 GPIO 布局（gpio_ctrl.h）
    CAP_GPIO_LAYOUT_RV1126=1              # RV1126 系列 GPIO 布局（gpio_ctrl.h）
    CAP_LIGHT_WHITE_ONLY=1                # 仅白光灯能力（pwm_ctrl.h / light_manager.cpp）
    CAP_PWM_NEED_POLARITY=1               # PWM 极性配置（pwm_utils.cpp）
    CAP_EVENT_AUDIO_PLAYBACK_V2=1         # 事件音频播放 V2（event_linkage.cpp）
    CAP_AUDIO_PLAYBACK_SLEEP_HALF=0       # 音频播放 half-sleep（event_linkage.cpp）
    CAP_SYSTEM_REBOOT_MUTE=1              # 系统重启静音处理（system_manage.cpp / event_resource.cpp）
    CAP_STORAGE_MMCBLK1=1                 # 存储 mmcblk1 路径逻辑（storage_manage.cpp）
    CAP_PROCESS_USE_PS_GREP=1             # 进程探测 ps|grep（process_manager.cpp）
    CAP_PROCESS_KILL_CROND_BEFORE_STREAM=1  # 拉流前处理 crond（process_manager.cpp）
    CAP_DAEMON_LOG_BY_SIZE=0              # 守护进程按大小滚动日志（daemon/main.cpp）
    CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE=1  # record 额外 include（ipc.cmake / record/CMakeLists.txt）
    CAP_RECORD_LINK_FDK_AAC=1             # record 链接 fdk-aac（ipc.cmake / record/CMakeLists.txt）
    CAP_AI_USE_SIMPLE_JSON=0              # AI Json 头选择（CVInferenceHISI.hpp）
    CAP_EXHIBITION_OSD_PANEL=1            # 展会版AI左上角汇总面板能力
    CAP_PROCESS_LOG_SWITCH=0              # stream/record/operation等进程的日志开关（目前只输出error级别）
    CAP_NETWORK_TELNET_SERVICE=0         # telnet 服务能力；发布版本不启动 telnetd
    CAP_NETWORK_FTP_SERVICE=0            # ftp 服务能力；发布版本不启动 uftpd
    CAP_RECORD_USE_MAIN_STREAM=1         # 录制使用主码流
)
set(DEVICE_PROFILE_TV_3881T_CMAKE_VARS
    IPC_CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE=1 # record 额外 include（ipc.cmake / record/CMakeLists.txt）
    IPC_CAP_RECORD_LINK_FDK_AAC=1            # record 链接 fdk-aac（ipc.cmake / record/CMakeLists.txt）
    IPC_CAP_EXHIBITION_OSD_PANEL=1           # 展会版AI左上角汇总面板能力
)

set(DEVICE_PROFILE_TV_3882TI_DEVICE_TYPE "TV-3882TI")
# TV-3882TI 设备能力画像：
# - 能力值与 TV-3881T 一致
# - 差异点：额外启用 SCENE_INTELLIGENT_ANALYSIS（大模型/智能分析相关）
set(DEVICE_PROFILE_TV_3882TI_DEFINES
    DEVICE_TV_3882TI
    SCENE_INTELLIGENCE                     # 场景智能能力开关（历史业务宏）
    SCENE_INTELLIGENT_ANALYSIS             # TV-3882TI 增强智能分析宏（历史业务宏）

    CAP_ALARM_IO=1
    CAP_AUDIO_INPUT_LINEIN=1
    CAP_AUDIO_OUTPUT_LINEOUT=1
    CAP_AUDIO_FMT_AAC=1
    CAP_AUDIO_FMT_G711U=1
    CAP_AUDIO_FMT_G711A=1
    CAP_AUDIO_FMT_PCM=1
    CAP_AUDIO_FMT_G726=0
    CAP_MOTION_REGION_GRID=1
    CAP_VIDEO_MAX_2_5K=0
    CAP_VIDEO_MAX_4K=1
    CAP_SMART_EVENT_PERF_LIMIT=0
    CAP_ISP_IR_SWITCH=0
    CAP_GPIO_LAYOUT_852_SERIES=0
    CAP_GPIO_LAYOUT_RV1126=1
    CAP_LIGHT_WHITE_ONLY=1
    CAP_PWM_NEED_POLARITY=1
    CAP_EVENT_AUDIO_PLAYBACK_V2=1
    CAP_AUDIO_PLAYBACK_SLEEP_HALF=0
    CAP_SYSTEM_REBOOT_MUTE=1
    CAP_STORAGE_MMCBLK1=1
    CAP_STORAGE_HAS_EMMC=1       # 内部 eMMC 始终存在，TF 卡检测需排除 mmcblk0
    CAP_PROCESS_USE_PS_GREP=1
    CAP_PROCESS_KILL_CROND_BEFORE_STREAM=1
    CAP_DAEMON_LOG_BY_SIZE=0
    CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE=1
    CAP_RECORD_LINK_FDK_AAC=1
    CAP_AI_USE_SIMPLE_JSON=0
    CAP_NETWORK_TELNET_SERVICE=0
    CAP_NETWORK_FTP_SERVICE=0
    CAP_PROCESS_LOG_SWITCH=0   
    CAP_RECORD_USE_MAIN_STREAM=1 # 录制使用主码流
)
set(DEVICE_PROFILE_TV_3882TI_CMAKE_VARS
    IPC_CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE=1
    IPC_CAP_RECORD_LINK_FDK_AAC=1
)

function(apply_device_profile device_type)
    set(_matched FALSE)
    foreach(_key IN LISTS DEVICE_PROFILE_KEYS)
        set(_expect_type "${DEVICE_PROFILE_${_key}_DEVICE_TYPE}")
        if(device_type STREQUAL _expect_type)
            set(_defines ${DEVICE_PROFILE_${_key}_DEFINES})
            add_compile_definitions(${_defines})

            foreach(_item IN LISTS DEVICE_PROFILE_${_key}_CMAKE_VARS)
                string(REPLACE "=" ";" _kv "${_item}")
                list(GET _kv 0 _name)
                list(GET _kv 1 _value)
                set(${_name} ${_value} PARENT_SCOPE)
            endforeach()

            set(_matched TRUE)
            message(STATUS "DEVICE_PROFILE = ${_key}")
            break()
        endif()
    endforeach()

    if(NOT _matched)
        message(FATAL_ERROR "Unsupported DEVICE_TYPE: ${device_type}")
    endif()
endfunction()
