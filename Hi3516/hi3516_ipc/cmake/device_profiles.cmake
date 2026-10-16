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
#   使用: hi3516_ipc/include/share_define.h,
#         share/ipc_share/control/task/sub_task/event_task.cpp,
#         share/ipc_share/control/business/event/core/event_alarm.cpp
# - CAP_AUDIO_INPUT_LINEIN: LineIn 输入能力
#   使用: hi3516_ipc/main_app/stream_media/audio/stream_audio.cpp
# - CAP_AUDIO_OUTPUT_LINEOUT: LineOut 输出能力
#   使用: hi3516_ipc/main_app/stream_media/audio/stream_audio.cpp
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
# - CAP_GPIO_LAYOUT_3852_SERIES: 3852 系列 GPIO 布局
#   使用: share/ipc_share/control/business/system/gpioCtrl/gpio_ctrl.h
# - CAP_GPIO_LAYOUT_RV1126: RV1126 系列 GPIO 布局
#   使用: share/ipc_share/control/business/system/gpioCtrl/gpio_ctrl.h
# - CAP_LIGHT_WHITE_ONLY: 仅白光灯能力
#   使用: hi3516_ipc/main_app/peripheral/peripheral_profile_builder.cpp
# - CAP_PWM_NEED_POLARITY: PWM 需要极性配置
#   使用: share/ipc_share/hardware/pwm_utils/pwm_utils.cpp
# - CAP_EVENT_AUDIO_PLAYBACK_V2: 事件音频播放 V2 路径
#   使用: share/ipc_share/control/business/event/linkage/event_linkage.cpp
# - CAP_AUDIO_PLAYBACK_SLEEP_HALF: 音频播放 half-sleep 策略
#   使用: share/ipc_share/control/business/event/linkage/event_linkage.cpp
# - CAP_SYSTEM_REBOOT_MUTE: 系统重启静音处理
#   使用: share/ipc_share/control/business/system/system_manage.cpp,
#         share/ipc_share/control/business/event/core/event_resource.cpp
# - CAP_STORAGE_MMCBLK1: 存储设备路径 mmcblk1 逻辑
#   使用: share/ipc_share/control/business/storage/storage_manage.cpp
# - CAP_PROCESS_USE_PS_GREP: 进程探测使用 ps|grep
#   使用: share/ipc_share/public_app/daemon/process_manager.cpp
# - CAP_PROCESS_KILL_CROND_BEFORE_STREAM: 拉流前处理 crond
#   使用: share/ipc_share/public_app/daemon/process_manager.cpp
# - CAP_DAEMON_LOG_BY_SIZE: 守护进程按文件大小滚动日志
#   使用: share/ipc_share/public_app/daemon/main.cpp
# - CAP_PROCESS_LOG_SWITCH: 进程日志级别切换（1=发布模式/WARN级别/syncPrintf关闭，0=开发模式/TRACE级别/syncPrintf开启）
#   使用: hi3516_ipc/main_app/stream_main.cpp,
#         share/ipc_share/public_app/record/main.cpp,
#         share/ipc_share/public_app/operation/main.cpp,
#         share/ipc_share/public_app/daemon/main.cpp,
#         hi3516_ipc/upgrade/main.cpp
# - CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE: record 需要额外 cam_share include
#   使用: hi3516_ipc/ipc.cmake, share/ipc_share/public_app/record/CMakeLists.txt
# - CAP_RECORD_LINK_FDK_AAC: record 需要链接 fdk-aac
#   使用: hi3516_ipc/ipc.cmake, share/ipc_share/public_app/record/CMakeLists.txt
# - CAP_AI_GARBAGE_DETECT: 垃圾暴露/垃圾满溢检测能力
#   使用: hi3516_ipc/main_app/ai_app/algorithm_mode/algorithm/garbage_detect/garbage_detect.hpp,
#         hi3516_ipc/main_app/ai_app/interface/algo_stream_deal.cpp,
#         share/ipc_share/common/define/event_define.h
# - CAP_AI_PEOPLE_STATISTICS: 人流统计/人员密度公共协议与配置能力
#   使用: share/ipc_share/common/define/alarm_define.h,
#         share/ipc_share/common/define/event_define.h,
#         share/ipc_share/control/business/event,
#         share/ipc_share/control/task/sub_task/event_task.cpp
# - CAP_UNIFIED_DETECTION: 统一检测层新链路总开关（由 IPC_CAP_UNIFIED_DETECTION 转换），
#   =0 全部算法走 Legacy，检测层源文件不编译进固件；=1 MD/OD/FACE 一并切换新链路并编译检测层。
#   展馆模型（CAP_AI_EXHIBITION_PEOPLE_FLOW）在此基础上二级门控：总开关=1 且展馆宏=1 才编译。
#   使用: hi3516_ipc/main_app/ai_app/algorithm_mode/detection/,
#         hi3516_ipc/main_app/ai_app/interface/algo_stream_deal.cpp/.h
# - CAP_AI_PEOPLE_DENSITY_LEGACY: 旧版人员密度检测能力，使用 people_head_detect 人头模型
#   使用: hi3516_ipc/main_app/ai_app/algorithm_mode/algorithm/people_head_detect
# - CAP_AI_PEOPLE_DENSITY_V2: 新版人员密度检测能力，使用 hvf_detect 人形模型
#   使用: hi3516_ipc/main_app/ai_app/algorithm_mode/algorithm/hvf_detect
# - CAP_AI_PEOPLE_DENSITY_PIPELINE: 人员密度新框架迁移能力（由 IPC_CAP_AI_PEOPLE_DENSITY_PIPELINE 转换），
#   =1 时展馆 YOLO 模型 head 类接入共享 pipeline，编译关闭 HVF 密度链路
#   使用: share/ipc_share/event/pipeline/process/people_density,
#         hi3516_ipc/main_app/ai_app/algorithm_mode/detection/exhibition_detect
# - CAP_UNIFIED_EVENT_PIPELINE: 全局事件链路（新事件结构）总开关（由 IPC_CAP_UNIFIED_EVENT_PIPELINE 转换），
#   =0 事件走旧事件结构或 per-event 宏控制；=1 事件走新事件结构（Converter->Dispatcher->Processor->OutputExecutor）。
#   跨算法类的迁移总开关，渐进替代 CAP_AI_PEOPLE_FLOW_PIPELINE /
#   CAP_AI_EXHIBITION_PEOPLE_FLOW / CAP_AI_PEOPLE_DENSITY_PIPELINE 等 per-event 宏，最终移除旧框架。
#   使用: hi3516_ipc/main_app/ai_app（事件处理迁移引用点，随迁移子任务逐步接入）,
#         share/ipc_share/event/pipeline
# - CAP_EXHIBITION_OSD_PANEL: 展会版左上角 AI 汇总面板能力，并拦截网页 OSD 设置
#   使用: hi3516_ipc/main_app/ai_app/common/common_process.cpp,
#         hi3516_ipc/main_app/stream_media/video/osd/osd_manage.cpp,
#         hi3516_ipc/main_app/stream_media/video/osd/overplay_draw.cpp,
#         share/ipc_share/control/task/sub_task/picture_task.cpp
# - CAP_AI_USE_SIMPLE_JSON: AI 使用 simple Json 头
#   使用: share/ai_share/AiModules/Inference/Hisilicon/CVInferenceHISI.hpp
# ISP参数映射、日夜阈值、Scene/DRC、Gamma和安装方向已迁移到Sensor目录下的INI配置。
# - CAP_NETWORK_4G: 4G 网络能力
#   使用: （预留，暂未实现）
# - CAP_NETWORK_WIFI: WIFI 网络能力
# - CAP_BLUETOOTH: 蓝牙/GATT 配网能力（由 IPC_CAP_BLUETOOTH 转换）
#   仅 TV-3852TL4G、TV-3852TLW 开启
#   使用: （预留，暂未实现）
# - CAP_RECORD_USE_MAIN_STREAM: 录制使用主码流（默认0=子码流，1=主码流）
#   使用: hi3516_ipc/main_app/stream_media/video/venc_channel_handler.cpp,
#         hi3516_ipc/main_app/stream_media/video/stream_video.cpp,
#         share/ipc_share/public_app/record/record_file/record_file.cpp
# - CAP_GARBAGE_STATION_PLATFORM: 垃圾站平台接入能力
#   使用: share/ipc_share/control/business/network/platform/platform_manager.h
# - CAP_NETWORK_TELNET_SERVICE: 启动 telnet 服务能力
#   使用: share/ipc_share/control/business/network/network_manage.cpp
# - CAP_NETWORK_FTP_SERVICE: 启动 ftp 服务能力
#   使用: share/ipc_share/control/business/network/network_manage.cpp
# - IPC_CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE: CMake 变量版 record include 能力
#   使用: hi3516_ipc/ipc.cmake, share/ipc_share/public_app/record/CMakeLists.txt
# - IPC_CAP_RECORD_LINK_FDK_AAC: CMake 变量版 record 链接能力
#   使用: hi3516_ipc/ipc.cmake, share/ipc_share/public_app/record/CMakeLists.txt
# - IPC_CAP_EXHIBITION_OSD_PANEL: CMake 变量版展会面板能力
#   使用: hi3516_ipc/main_app/ai_app/common/common.cmake
# - CAP_AI_FACE_COMPARE: 启用人脸比对能力
#   使用: hi3516_ipc/main_app/ai_app/algorithm_mode/algorithm/face_detect/internal/processors/feature/face_feature_processor.cpp
# - CAP_IO_EXTERNAL_DDR_00S=1            #外置DDR适配io
#   使用: /hi3516_ipc/hi3516_ipc/main_app/stream_media/audio/ao/stream_ao.h
# - CAP_RTMP_PUSH: RTMP 推流能力（独立于平台接入，允许有平台接入但无 RTMP）
#   使用: share/ipc_share/push_stream/push_stream.h,
#         share/ipc_share/push_stream/push_stream.cpp,
#         hi3516_ipc/main_app/stream_media/video/stream_video.cpp,
#         share/ipc_share/control/business/network/platform/platform_manager.cpp
# - CAP_RTSP_HIGH_CONCURRENCY: RTSP 会话高并发能力（0=Hi3516 默认4路总额，1=高配8路总额）
#   使用: share/ipc_share/push_stream/rtsp/rtsp_server.h
# - IPC_CAP_RTMP_PUSH: CMake 变量版 RTMP 推流能力
#   使用: share/ipc_share/push_stream/push_stream.cmake

set(DEVICE_PROFILE_KEYS
    TV_3852T
    TV_3852H
    TV_3852TL
    TV_3852HL
    TV_3852TL4G
    TV_3852TLW
    TV_3852HZT
)

set(DEVICE_PROFILE_TV_3852T_DEVICE_TYPE "TV-3852T")
# TV-3852T 设备能力画像（3516CV610 基线）
set(DEVICE_PROFILE_TV_3852T_DEFINES
    DEVICE_TV_3852T
    CAP_ALARM_IO=1                       # 报警IO能力（event_task.cpp / event_alarm.cpp / share_define.h）
    CAP_AUDIO_INPUT_LINEIN=1             # LineIn 输入能力（stream_audio.cpp）
    CAP_AUDIO_OUTPUT_LINEOUT=1           # LineOut 输出能力（stream_audio.cpp）
    CAP_AUDIO_FMT_AAC=1                  # AAC 编码能力（av_configure.cpp）
    CAP_AUDIO_FMT_G711U=1                # G.711ulaw 编码能力（av_configure.cpp）
    CAP_AUDIO_FMT_G711A=1                # G.711alaw 编码能力（av_configure.cpp）
    CAP_AUDIO_FMT_PCM=1                  # PCM 编码能力（av_configure.cpp）
    CAP_AUDIO_FMT_G726=0                 # G.726 编码能力（av_configure.cpp）
    CAP_MOTION_REGION_GRID=1             # 移动侦测网格能力（alarm_define.h）
    CAP_VIDEO_MAX_2_5K=1                 # 视频最大分辨率 2.5K（video_config_compat.h）
    CAP_VIDEO_MAX_4K=0                   # 视频最大分辨率 4K（video_config_compat.h）
    CAP_SMART_EVENT_PERF_LIMIT=1         # 智能事件性能限制（system_task.cpp）
    CAP_ISP_IR_SWITCH=1                  # ISP 红外切换（isp_manage.cpp）
    CAP_GPIO_LAYOUT_3852_SERIES=1         # 3852 系列 GPIO 布局（gpio_ctrl.h）
    CAP_GPIO_LAYOUT_RV1126=0             # RV1126 系列 GPIO 布局（gpio_ctrl.h）
    CAP_LIGHT_WHITE_ONLY=0               # 仅白光灯能力（peripheral_profile_builder.cpp）
    CAP_PWM_NEED_POLARITY=0              # PWM 极性配置（pwm_utils.cpp）
    CAP_EVENT_AUDIO_PLAYBACK_V2=0        # 事件音频播放 V2（event_linkage.cpp）
    CAP_AUDIO_PLAYBACK_SLEEP_HALF=1      # 音频播放 half-sleep（event_linkage.cpp）
    CAP_SYSTEM_REBOOT_MUTE=0             # 系统重启静音处理（system_manage.cpp / event_resource.cpp）
    CAP_STORAGE_MMCBLK1=0                # 存储 mmcblk1 路径逻辑（storage_manage.cpp）
    CAP_PROCESS_USE_PS_GREP=0            # 进程探测 ps|grep（process_manager.cpp）
    CAP_PROCESS_KILL_CROND_BEFORE_STREAM=0  # 拉流前处理 crond（process_manager.cpp）
    CAP_DAEMON_LOG_BY_SIZE=1             # 守护进程按大小滚动日志（daemon/main.cpp）
    CAP_PROCESS_LOG_SWITCH=0             # 进程日志级别切换（0=开发模式/TRACE，1=发布模式/WARN）
    CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE=0 # record 额外 include（ipc.cmake / record/CMakeLists.txt）
    CAP_RECORD_LINK_FDK_AAC=0            # record 链接 fdk-aac（ipc.cmake / record/CMakeLists.txt）
    CAP_AI_GARBAGE_DETECT=0              # 垃圾暴露/垃圾满溢检测能力（garbage_detect.hpp / algo_stream_deal.cpp / event_define.h）
    CAP_AI_PEOPLE_STATISTICS=0           # 人流统计/人员密度公共协议与配置能力
    CAP_AI_PEOPLE_DENSITY_LEGACY=0       # 旧版人员密度，人头模型实现
    CAP_AI_PEOPLE_DENSITY_V2=0           # 新版人员密度，HVF 人形模型实现
    CAP_EXHIBITION_OSD_PANEL=0           # 展会版AI左上角汇总面板能力
    CAP_AI_USE_SIMPLE_JSON=1             # AI 使用 simple Json 头（CVInferenceHISI.hpp）
    CAP_GPIO_IR_CUT_JSON=1               # gpio控制ir_cut方式（gpio_ctrl.cpp）
    CAP_RECORD_USE_MAIN_STREAM=0         # 录制使用主码流
    CAP_NETWORK_TELNET_SERVICE=0         # telnet 服务能力；发布版本不启动 telnetd
    CAP_NETWORK_FTP_SERVICE=0            # ftp 服务能力；发布版本不启动 uftpd
    CAP_RTSP_HIGH_CONCURRENCY=0          # RTSP 高并发能力（普通 Hi3516）
    CAP_RTMP_PUSH=0                      # RTMP 推流能力
)
set(DEVICE_PROFILE_TV_3852T_CMAKE_VARS
    IPC_CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE=0 # record 额外 include（ipc.cmake / record/CMakeLists.txt）
    IPC_CAP_RECORD_LINK_FDK_AAC=0            # record 链接 fdk-aac（ipc.cmake / record/CMakeLists.txt）
    IPC_CAP_EXHIBITION_OSD_PANEL=0           # 展会版AI左上角汇总面板能力
    IPC_CAP_RTMP_PUSH=0                      # RTMP 推流能力
    IPC_CAP_AI_PEOPLE_FLOW_PIPELINE=0        # 人流统计新框架迁移管线（0=Legacy，1=New）
    IPC_CAP_UNIFIED_DETECTION=0            # 统一检测层新链路总开关（0=Legacy 全走旧链路，1=MD/OD/FACE 一并切新链路）
    IPC_CAP_AI_EXHIBITION_PEOPLE_FLOW=0      # 展馆人流统计自研模型（0=HVF，1=展馆YOLO）
    IPC_CAP_AI_PEOPLE_DENSITY_PIPELINE=0     # 人员密度新框架迁移（0=HVF V2，1=展馆YOLO head）
    IPC_CAP_UNIFIED_EVENT_PIPELINE=0         # 全局事件链路（新事件结构）总开关（0=Legacy，1=New）
)

set(DEVICE_PROFILE_TV_3852H_DEVICE_TYPE "TV-3852H")
# TV-3852H 设备能力画像：
# - 继承 852T 基线
# - 差异点：仅保留 MIC/SPEAKER，关闭报警 IO 与 LineIn/LineOut
set(DEVICE_PROFILE_TV_3852H_DEFINES
    DEVICE_TV_3852H
    CAP_ALARM_IO=0
    CAP_AUDIO_INPUT_LINEIN=0
    CAP_AUDIO_OUTPUT_LINEOUT=0
    CAP_AUDIO_FMT_AAC=1
    CAP_AUDIO_FMT_G711U=1
    CAP_AUDIO_FMT_G711A=1
    CAP_AUDIO_FMT_PCM=1
    CAP_AUDIO_FMT_G726=0
    CAP_MOTION_REGION_GRID=1
    CAP_VIDEO_MAX_2_5K=1
    CAP_VIDEO_MAX_4K=0
    CAP_SMART_EVENT_PERF_LIMIT=1
    CAP_ISP_IR_SWITCH=1
    CAP_GPIO_LAYOUT_3852_SERIES=1
    CAP_GPIO_LAYOUT_RV1126=0
    CAP_LIGHT_WHITE_ONLY=0
    CAP_PWM_NEED_POLARITY=0
    CAP_EVENT_AUDIO_PLAYBACK_V2=0
    CAP_AUDIO_PLAYBACK_SLEEP_HALF=1
    CAP_SYSTEM_REBOOT_MUTE=0
    CAP_STORAGE_MMCBLK1=0
    CAP_PROCESS_USE_PS_GREP=0
    CAP_PROCESS_KILL_CROND_BEFORE_STREAM=0
    CAP_DAEMON_LOG_BY_SIZE=1
    CAP_PROCESS_LOG_SWITCH=0             # 进程日志级别切换（0=开发模式/TRACE，1=发布模式/WARN）
    CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE=0
    CAP_RECORD_LINK_FDK_AAC=0
    CAP_AI_GARBAGE_DETECT=0
    CAP_AI_PEOPLE_STATISTICS=0
    CAP_AI_PEOPLE_DENSITY_LEGACY=0
    CAP_AI_PEOPLE_DENSITY_V2=0
    CAP_AI_USE_SIMPLE_JSON=1
    CAP_GPIO_IR_CUT_JSON=0
    CAP_RECORD_USE_MAIN_STREAM=0         # 录制使用主码流
    CAP_NETWORK_TELNET_SERVICE=0         # telnet 服务能力；发布版本不启动 telnetd
    CAP_NETWORK_FTP_SERVICE=0            # ftp 服务能力；发布版本不启动 uftpd
    CAP_RTSP_HIGH_CONCURRENCY=0          # RTSP 高并发能力（普通 Hi3516）
    CAP_RTMP_PUSH=0                      # RTMP 推流能力
)
set(DEVICE_PROFILE_TV_3852H_CMAKE_VARS
    IPC_CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE=0
    IPC_CAP_RECORD_LINK_FDK_AAC=0
    IPC_CAP_RTMP_PUSH=0                      # RTMP 推流能力
    IPC_CAP_AI_PEOPLE_FLOW_PIPELINE=0        # 人流统计新框架迁移管线（0=Legacy，1=New）
    IPC_CAP_UNIFIED_DETECTION=0            # 统一检测层新链路总开关（0=Legacy 全走旧链路，1=MD/OD/FACE 一并切新链路）
    IPC_CAP_AI_EXHIBITION_PEOPLE_FLOW=0      # 展馆人流统计自研模型（0=HVF，1=展馆YOLO）
    IPC_CAP_AI_PEOPLE_DENSITY_PIPELINE=0     # 人员密度新框架迁移（0=HVF V2，1=展馆YOLO head）
    IPC_CAP_UNIFIED_EVENT_PIPELINE=0         # 全局事件链路（新事件结构）总开关（0=Legacy，1=New）
)

# ========== 智能垃圾站系列型号 ==========

set(DEVICE_PROFILE_TV_3852TL_DEVICE_TYPE "TV-3852TL")
# TV-3852TL 设备能力画像（智能垃圾站筒型海螺型有线）
# - 继承 852T 基线
# - 差异点：开启垃圾检测能力，有线网络
set(DEVICE_PROFILE_TV_3852TL_DEFINES
    DEVICE_TV_3852TL
    CAP_ALARM_IO=0
    CAP_AUDIO_INPUT_LINEIN=1
    CAP_AUDIO_OUTPUT_LINEOUT=1
    CAP_AUDIO_FMT_AAC=1
    CAP_AUDIO_FMT_G711U=1
    CAP_AUDIO_FMT_G711A=1
    CAP_AUDIO_FMT_PCM=1
    CAP_AUDIO_FMT_G726=0
    CAP_MOTION_REGION_GRID=1
    CAP_VIDEO_MAX_2_5K=1
    CAP_VIDEO_MAX_4K=0
    CAP_SMART_EVENT_PERF_LIMIT=1
    CAP_ISP_IR_SWITCH=1
    CAP_GPIO_LAYOUT_3852_SERIES=1
    CAP_GPIO_LAYOUT_RV1126=0
    CAP_LIGHT_WHITE_ONLY=0
    CAP_PWM_NEED_POLARITY=0
    CAP_EVENT_AUDIO_PLAYBACK_V2=0
    CAP_AUDIO_PLAYBACK_SLEEP_HALF=1
    CAP_SYSTEM_REBOOT_MUTE=0
    CAP_STORAGE_MMCBLK1=0
    CAP_PROCESS_USE_PS_GREP=0
    CAP_PROCESS_KILL_CROND_BEFORE_STREAM=0
    CAP_DAEMON_LOG_BY_SIZE=1
    CAP_PROCESS_LOG_SWITCH=0             # 进程日志级别切换（0=开发模式/TRACE，1=发布模式/WARN）
    CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE=0
    CAP_RECORD_LINK_FDK_AAC=0
    CAP_AI_GARBAGE_DETECT=1              # 垃圾暴露/垃圾满溢检测能力
    CAP_AI_SMOKE_FIRE_DETECT=1           # 烟火识别功能
    CAP_AI_PEOPLE_STATISTICS=0           # 人流统计/人员密度公共协议与配置能力
    CAP_AI_PEOPLE_DENSITY_LEGACY=0       # 旧版人员密度，人头模型实现
    CAP_AI_PEOPLE_DENSITY_V2=0           # 新版人员密度，HVF 人形模型实现
    CAP_AI_FACE_COMPARE=1                # 人脸比对检测能力
    CAP_AI_USE_SIMPLE_JSON=1
    CAP_GPIO_IR_CUT_JSON=1
    CAP_NETWORK_TELNET_SERVICE=0         # telnet 服务能力；发布版本不启动 telnetd
    CAP_NETWORK_FTP_SERVICE=0            # ftp 服务能力；发布版本不启动 uftpd
    CAP_NETWORK_4G=0                     # 4G网络（预留）
    CAP_NETWORK_WIFI=0                   # WIFI网络（预留）
    CAP_RECORD_USE_MAIN_STREAM=0         # 录制使用主码流
    CAP_GARBAGE_STATION_PLATFORM=1       # 垃圾站平台接入能力
    CAP_NETWORK_TELNET_SERVICE=0         # telnet 服务能力；发布版本不启动 telnetd
    CAP_NETWORK_FTP_SERVICE=0            # ftp 服务能力；发布版本不启动 uftpd
    CAP_IO_EXTERNAL_DDR_00S=0            #外置DDR适配io
    CAP_RTSP_HIGH_CONCURRENCY=0          # RTSP 高并发能力（普通 Hi3516）
    CAP_RTMP_PUSH=0                      # RTMP 推流能力（TL 型号不需要 RTMP）
)
set(DEVICE_PROFILE_TV_3852TL_CMAKE_VARS
    IPC_CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE=0
    IPC_CAP_RECORD_LINK_FDK_AAC=0
    IPC_CAP_EXHIBITION_OSD_PANEL=0           # 展会版AI左上角汇总面板能力
    IPC_CAP_RTMP_PUSH=0                      # RTMP 推流能力
    IPC_CAP_AI_PEOPLE_FLOW_PIPELINE=0        # 人流统计新框架迁移管线（0=Legacy，1=New）
    IPC_CAP_UNIFIED_DETECTION=0             # 统一检测层新链路总开关（0=Legacy 全走旧链路，1=MD/OD/FACE 一并切新链路）
    IPC_CAP_AI_EXHIBITION_PEOPLE_FLOW=0      # 展馆人流统计自研模型（0=HVF，1=展馆YOLO）
    IPC_CAP_AI_PEOPLE_DENSITY_PIPELINE=0     # 人员密度新框架迁移（0=HVF V2，1=展馆YOLO head）
    IPC_CAP_UNIFIED_EVENT_PIPELINE=0         # 全局事件链路（新事件结构）总开关（0=Legacy，1=New）
)

set(DEVICE_PROFILE_TV_3852HL_DEVICE_TYPE "TV-3852HL")
# TV-3852HL 设备能力画像（智能垃圾站海螺型有线）
# - 继承 852H 基线
# - 差异点：开启垃圾检测能力，有线网络
set(DEVICE_PROFILE_TV_3852HL_DEFINES
    DEVICE_TV_3852HL
    CAP_ALARM_IO=0
    CAP_AUDIO_INPUT_LINEIN=0
    CAP_AUDIO_OUTPUT_LINEOUT=0
    CAP_AUDIO_FMT_AAC=1
    CAP_AUDIO_FMT_G711U=1
    CAP_AUDIO_FMT_G711A=1
    CAP_AUDIO_FMT_PCM=1
    CAP_AUDIO_FMT_G726=0
    CAP_MOTION_REGION_GRID=1
    CAP_VIDEO_MAX_2_5K=1
    CAP_VIDEO_MAX_4K=0
    CAP_SMART_EVENT_PERF_LIMIT=1
    CAP_ISP_IR_SWITCH=1
    CAP_GPIO_LAYOUT_3852_SERIES=1
    CAP_GPIO_LAYOUT_RV1126=0
    CAP_LIGHT_WHITE_ONLY=0
    CAP_PWM_NEED_POLARITY=0
    CAP_EVENT_AUDIO_PLAYBACK_V2=0
    CAP_AUDIO_PLAYBACK_SLEEP_HALF=1
    CAP_SYSTEM_REBOOT_MUTE=0
    CAP_STORAGE_MMCBLK1=0
    CAP_PROCESS_USE_PS_GREP=0
    CAP_PROCESS_KILL_CROND_BEFORE_STREAM=0
    CAP_DAEMON_LOG_BY_SIZE=1
    CAP_PROCESS_LOG_SWITCH=0             # 进程日志级别切换（0=开发模式/TRACE，1=发布模式/WARN）
    CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE=0
    CAP_RECORD_LINK_FDK_AAC=0
    CAP_AI_GARBAGE_DETECT=1              # 垃圾暴露/垃圾满溢检测能力
    CAP_AI_SMOKE_FIRE_DETECT=1           # 烟火识别功能
    CAP_AI_PEOPLE_STATISTICS=0           # 人流统计/人员密度公共协议与配置能力
    CAP_AI_PEOPLE_DENSITY_LEGACY=0       # 旧版人员密度，人头模型实现
    CAP_AI_PEOPLE_DENSITY_V2=0           # 新版人员密度，HVF 人形模型实现
    CAP_AI_FACE_COMPARE=1                # 人脸比对检测能力
    CAP_AI_USE_SIMPLE_JSON=1
    CAP_NETWORK_4G=0                     # 4G网络（预留）
    CAP_NETWORK_WIFI=0                   # WIFI网络（预留）
    CAP_RECORD_USE_MAIN_STREAM=0         # 录制使用主码流
    CAP_GARBAGE_STATION_PLATFORM=1       # 垃圾站平台接入能力
    CAP_IO_EXTERNAL_DDR_00S=0            #外置DDR适配io
    CAP_RTSP_HIGH_CONCURRENCY=0          # RTSP 高并发能力（普通 Hi3516）
    CAP_RTMP_PUSH=0                      # RTMP 推流能力（HL 型号不需要 RTMP）
)
set(DEVICE_PROFILE_TV_3852HL_CMAKE_VARS
    IPC_CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE=0
    IPC_CAP_RECORD_LINK_FDK_AAC=0
    IPC_CAP_RTMP_PUSH=0                      # RTMP 推流能力
    IPC_CAP_AI_PEOPLE_FLOW_PIPELINE=0        # 人流统计新框架迁移管线（0=Legacy，1=New）
    IPC_CAP_UNIFIED_DETECTION=0             # 统一检测层新链路总开关（0=Legacy 全走旧链路，1=MD/OD/FACE 一并切新链路）
    IPC_CAP_AI_EXHIBITION_PEOPLE_FLOW=0      # 展馆人流统计自研模型（0=HVF，1=展馆YOLO）
    IPC_CAP_AI_PEOPLE_DENSITY_PIPELINE=0     # 人员密度新框架迁移（0=HVF V2，1=展馆YOLO head）
    IPC_CAP_UNIFIED_EVENT_PIPELINE=0         # 全局事件链路（新事件结构）总开关（0=Legacy，1=New）
)

set(DEVICE_PROFILE_TV_3852TL4G_DEVICE_TYPE "TV-3852TL4G")
# TV-3852TL4G 设备能力画像（智能垃圾站筒型海螺型4G）
# - 继承 852T 基线
# - 差异点：开启垃圾检测能力，4G网络能力宏预留
set(DEVICE_PROFILE_TV_3852TL4G_DEFINES
    DEVICE_TV_3852TL4G
    CAP_ALARM_IO=0
    CAP_AUDIO_INPUT_LINEIN=1
    CAP_AUDIO_OUTPUT_LINEOUT=1
    CAP_AUDIO_FMT_AAC=1
    CAP_AUDIO_FMT_G711U=1
    CAP_AUDIO_FMT_G711A=1
    CAP_AUDIO_FMT_PCM=1
    CAP_AUDIO_FMT_G726=0
    CAP_MOTION_REGION_GRID=1
    CAP_VIDEO_MAX_2_5K=1
    CAP_VIDEO_MAX_4K=0
    CAP_SMART_EVENT_PERF_LIMIT=1
    CAP_ISP_IR_SWITCH=1
    CAP_GPIO_LAYOUT_3852_SERIES=1
    CAP_GPIO_LAYOUT_RV1126=0
    CAP_LIGHT_WHITE_ONLY=0
    CAP_PWM_NEED_POLARITY=0
    CAP_EVENT_AUDIO_PLAYBACK_V2=0
    CAP_AUDIO_PLAYBACK_SLEEP_HALF=1
    CAP_SYSTEM_REBOOT_MUTE=0
    CAP_STORAGE_MMCBLK1=0
    CAP_PROCESS_USE_PS_GREP=0
    CAP_PROCESS_KILL_CROND_BEFORE_STREAM=0
    CAP_DAEMON_LOG_BY_SIZE=1
    CAP_PROCESS_LOG_SWITCH=0             # 进程日志级别切换（0=开发模式/TRACE，1=发布模式/WARN）
    CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE=0
    CAP_RECORD_LINK_FDK_AAC=0
    CAP_AI_GARBAGE_DETECT=1              # 垃圾暴露/垃圾满溢检测能力
    CAP_AI_SMOKE_FIRE_DETECT=1           # 烟火识别功能
    CAP_AI_PEOPLE_STATISTICS=0           # 人流统计/人员密度公共协议与配置能力
    CAP_AI_PEOPLE_DENSITY_LEGACY=0       # 旧版人员密度，人头模型实现
    CAP_AI_PEOPLE_DENSITY_V2=0           # 新版人员密度，HVF 人形模型实现
    CAP_AI_FACE_COMPARE=1                # 人脸比对检测能力
    CAP_AI_USE_SIMPLE_JSON=1
    CAP_GPIO_IR_CUT_JSON=1
    CAP_NETWORK_4G=1                     # 4G网络（预留）
    CAP_NETWORK_WIFI=0                   # WIFI网络（预留）
    CAP_RECORD_USE_MAIN_STREAM=0         # 录制使用主码流
    CAP_GARBAGE_STATION_PLATFORM=1       # 垃圾站平台接入能力
    CAP_NETWORK_TELNET_SERVICE=0         # telnet 服务能力；发布版本不启动 telnetd
    CAP_NETWORK_FTP_SERVICE=0            # ftp 服务能力；发布版本不启动 uftpd
    CAP_IO_EXTERNAL_DDR_00S=1            #外置DDR适配io
    CAP_RTSP_HIGH_CONCURRENCY=0          # RTSP 高并发能力（普通 Hi3516）
    CAP_RTMP_PUSH=1                      # RTMP 推流能力
)
set(DEVICE_PROFILE_TV_3852TL4G_CMAKE_VARS
    IPC_CAP_BLUETOOTH=1                      # 蓝牙/GATT 配网能力
    IPC_CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE=0
    IPC_CAP_RECORD_LINK_FDK_AAC=0
    IPC_CAP_EXHIBITION_OSD_PANEL=0           # 展会版AI左上角汇总面板能力
    IPC_CAP_RTMP_PUSH=1                      # RTMP 推流能力
    IPC_CAP_AI_PEOPLE_FLOW_PIPELINE=0        # 人流统计新框架迁移管线（0=Legacy，1=New）
    IPC_CAP_UNIFIED_DETECTION=0            # 统一检测层新链路总开关（0=Legacy 全走旧链路，1=MD/OD/FACE 一并切新链路）
    IPC_CAP_AI_EXHIBITION_PEOPLE_FLOW=0      # 展馆人流统计自研模型（0=HVF，1=展馆YOLO）
    IPC_CAP_AI_PEOPLE_DENSITY_PIPELINE=0     # 人员密度新框架迁移（0=HVF V2，1=展馆YOLO head）
    IPC_CAP_UNIFIED_EVENT_PIPELINE=0         # 全局事件链路（新事件结构）总开关（0=Legacy，1=New）
)

set(DEVICE_PROFILE_TV_3852TLW_DEVICE_TYPE "TV-3852TLW")
# TV-3852TLW 设备能力画像（智能垃圾站筒型海螺型WIFI）
# - 继承 852T 基线
# - 差异点：开启垃圾检测能力，WIFI网络能力宏预留
set(DEVICE_PROFILE_TV_3852TLW_DEFINES
    DEVICE_TV_3852TLW
    CAP_ALARM_IO=0
    CAP_AUDIO_INPUT_LINEIN=1
    CAP_AUDIO_OUTPUT_LINEOUT=1
    CAP_AUDIO_FMT_AAC=1
    CAP_AUDIO_FMT_G711U=1
    CAP_AUDIO_FMT_G711A=1
    CAP_AUDIO_FMT_PCM=1
    CAP_AUDIO_FMT_G726=0
    CAP_MOTION_REGION_GRID=1
    CAP_VIDEO_MAX_2_5K=1
    CAP_VIDEO_MAX_4K=0
    CAP_SMART_EVENT_PERF_LIMIT=1
    CAP_ISP_IR_SWITCH=1
    CAP_GPIO_LAYOUT_3852_SERIES=1
    CAP_GPIO_LAYOUT_RV1126=0
    CAP_LIGHT_WHITE_ONLY=0
    CAP_PWM_NEED_POLARITY=0
    CAP_EVENT_AUDIO_PLAYBACK_V2=0
    CAP_AUDIO_PLAYBACK_SLEEP_HALF=1
    CAP_SYSTEM_REBOOT_MUTE=0
    CAP_STORAGE_MMCBLK1=0
    CAP_PROCESS_USE_PS_GREP=0
    CAP_PROCESS_KILL_CROND_BEFORE_STREAM=0
    CAP_DAEMON_LOG_BY_SIZE=1
    CAP_PROCESS_LOG_SWITCH=0             # 进程日志级别切换（0=开发模式/TRACE，1=发布模式/WARN）
    CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE=0
    CAP_RECORD_LINK_FDK_AAC=0
    CAP_AI_GARBAGE_DETECT=1              # 垃圾暴露/垃圾满溢检测能力
    CAP_AI_SMOKE_FIRE_DETECT=1           # 烟火识别功能
    CAP_AI_PEOPLE_STATISTICS=0           # 人流统计/人员密度公共协议与配置能力
    CAP_AI_PEOPLE_DENSITY_LEGACY=0       # 旧版人员密度，人头模型实现
    CAP_AI_PEOPLE_DENSITY_V2=0           # 新版人员密度，HVF 人形模型实现
    CAP_AI_FACE_COMPARE=1                # 人脸比对检测能力
    CAP_AI_USE_SIMPLE_JSON=1
    CAP_GPIO_IR_CUT_JSON=1
    CAP_NETWORK_4G=0                     # 4G网络（预留）
    CAP_NETWORK_WIFI=1                   # WIFI网络（预留）
    CAP_RECORD_USE_MAIN_STREAM=0         # 录制使用主码流
    CAP_GARBAGE_STATION_PLATFORM=1       # 垃圾站平台接入能力
    CAP_NETWORK_TELNET_SERVICE=0         # telnet 服务能力；发布版本不启动 telnetd
    CAP_NETWORK_FTP_SERVICE=0            # ftp 服务能力；发布版本不启动 uftpd
    CAP_IO_EXTERNAL_DDR_00S=1            #外置DDR适配io
    CAP_RTSP_HIGH_CONCURRENCY=0          # RTSP 高并发能力（普通 Hi3516）
    CAP_RTMP_PUSH=1                      # RTMP 推流能力
)
set(DEVICE_PROFILE_TV_3852TLW_CMAKE_VARS
    IPC_CAP_BLUETOOTH=1                      # 蓝牙/GATT 配网能力（0=不编译且不打包蓝牙依赖）
    IPC_CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE=0
    IPC_CAP_RECORD_LINK_FDK_AAC=0
    IPC_CAP_EXHIBITION_OSD_PANEL=0           # 展会版AI左上角汇总面板能力
    IPC_CAP_RTMP_PUSH=1                      # RTMP 推流能力
    IPC_CAP_AI_PEOPLE_FLOW_PIPELINE=0        # 人流统计新框架迁移管线（0=Legacy，1=New）
    IPC_CAP_UNIFIED_DETECTION=0            # 统一检测层新链路总开关（0=Legacy 全走旧链路，1=MD/OD/FACE 一并切新链路）
    IPC_CAP_AI_EXHIBITION_PEOPLE_FLOW=0      # 展馆人流统计自研模型（0=HVF，1=展馆YOLO）
    IPC_CAP_AI_PEOPLE_DENSITY_PIPELINE=0     # 人员密度新框架迁移（0=HVF V2，1=展馆YOLO head）
    IPC_CAP_UNIFIED_EVENT_PIPELINE=0         # 全局事件链路（新事件结构）总开关（0=Legacy，1=New）
)

# 展厅特殊版本
set(DEVICE_PROFILE_TV_3852HZT_DEVICE_TYPE "TV-3852HZT")
# TV-3852HZT 设备能力画像：
# - 继承 3852H 基线
# - 实际产品型号仍显示为 TV-3852H，仅用于临时构建区分能力宏
set(DEVICE_PROFILE_TV_3852HZT_DEFINES
    DEVICE_TV_3852H
    CAP_ALARM_IO=0
    CAP_AUDIO_INPUT_LINEIN=0
    CAP_AUDIO_OUTPUT_LINEOUT=0
    CAP_AUDIO_FMT_AAC=1
    CAP_AUDIO_FMT_G711U=1
    CAP_AUDIO_FMT_G711A=1
    CAP_AUDIO_FMT_PCM=1
    CAP_AUDIO_FMT_G726=0
    CAP_MOTION_REGION_GRID=1
    CAP_VIDEO_MAX_2_5K=1
    CAP_VIDEO_MAX_4K=0
    CAP_SMART_EVENT_PERF_LIMIT=1
    CAP_ISP_IR_SWITCH=1
    CAP_GPIO_LAYOUT_3852_SERIES=1
    CAP_GPIO_LAYOUT_RV1126=0
    CAP_LIGHT_WHITE_ONLY=0
    CAP_PWM_NEED_POLARITY=0
    CAP_EVENT_AUDIO_PLAYBACK_V2=0
    CAP_AUDIO_PLAYBACK_SLEEP_HALF=1
    CAP_SYSTEM_REBOOT_MUTE=0
    CAP_STORAGE_MMCBLK1=0
    CAP_PROCESS_USE_PS_GREP=0
    CAP_PROCESS_KILL_CROND_BEFORE_STREAM=0
    CAP_DAEMON_LOG_BY_SIZE=1
    CAP_PROCESS_LOG_SWITCH=0             # 进程日志级别切换（0=开发模式/TRACE，1=发布模式/WARN）
    CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE=0
    CAP_RECORD_LINK_FDK_AAC=0
    CAP_AI_GARBAGE_DETECT=0
    CAP_AI_PEOPLE_STATISTICS=1              # 人流统计/人员密度公共协议与配置能力
    CAP_AI_PEOPLE_DENSITY_LEGACY=0          # 旧版人员密度，人头模型实现
    CAP_AI_PEOPLE_DENSITY_V2=1              # 新版人员密度，HVF 人形模型实现
    CAP_EXHIBITION_OSD_PANEL=0              # 展会版AI左上角汇总面板能力
    CAP_AI_USE_SIMPLE_JSON=1
    CAP_GPIO_IR_CUT_JSON=0
    CAP_RECORD_USE_MAIN_STREAM=0         # 录制使用主码流
    CAP_NETWORK_TELNET_SERVICE=1
    CAP_NETWORK_FTP_SERVICE=1
    CAP_RTSP_HIGH_CONCURRENCY=0          # RTSP 高并发能力（普通 Hi3516）
    CAP_RTMP_PUSH=0                      # RTMP 推流能力
)
set(DEVICE_PROFILE_TV_3852HZT_CMAKE_VARS
    IPC_CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE=0
    IPC_CAP_RECORD_LINK_FDK_AAC=0
    IPC_CAP_EXHIBITION_OSD_PANEL=0           # 展会版AI左上角汇总面板能力
    IPC_CAP_RTMP_PUSH=0                      # RTMP 推流能力
    IPC_CAP_AI_PEOPLE_FLOW_PIPELINE=1        # 人流统计新框架迁移管线（0=Legacy，1=New）
    IPC_CAP_UNIFIED_DETECTION=1             # 统一检测层新链路总开关（0=Legacy 全走旧链路，1=MD/OD/FACE 一并切新链路）
    IPC_CAP_AI_EXHIBITION_PEOPLE_FLOW=1      # 展馆人流统计自研模型（0=HVF，1=展馆YOLO）
    IPC_CAP_AI_PEOPLE_DENSITY_PIPELINE=1     # 人员密度新框架迁移（0=HVF V2，1=展馆YOLO head）
    IPC_CAP_UNIFIED_EVENT_PIPELINE=1         # 全局事件链路（新事件结构）总开关（0=Legacy，1=New）
)

# 画像能力宏校验并下发编译定义（IPC_CAP_* -> CAP_*）。
# 供 apply_device_profile 末尾调用：此时画像变量已 set 到调用者作用域（macro 无独立作用域）。
# 新增画像能力宏时在此追加校验与下发，ipc.cmake 不再需要同步修改。
macro(validate_and_export_caps)
    # 蓝牙/GATT 配网能力
    # 唯一配置入口是 DEVICE_PROFILE_*_CMAKE_VARS 中的 IPC_CAP_BLUETOOTH。
    # 未显式声明的型号默认关闭；缓存值同时供固件打包脚本读取。
    if(NOT DEFINED IPC_CAP_BLUETOOTH)
        set(IPC_CAP_BLUETOOTH 0)
    endif()
    if(NOT IPC_CAP_BLUETOOTH EQUAL 0 AND NOT IPC_CAP_BLUETOOTH EQUAL 1)
        message(FATAL_ERROR "IPC_CAP_BLUETOOTH 取值非法: ${IPC_CAP_BLUETOOTH}，只允许 0/1")
    endif()
    set(IPC_CAP_BLUETOOTH "${IPC_CAP_BLUETOOTH}" CACHE INTERNAL
        "Whether Bluetooth/GATT provisioning is compiled and packaged" FORCE)
    add_compile_definitions(CAP_BLUETOOTH=${IPC_CAP_BLUETOOTH})
    message(STATUS "IPC_CAP_BLUETOOTH = ${IPC_CAP_BLUETOOTH}")

    # 仅启用蓝牙时创建 GDBus/GLib 链接目标。关闭时 operation 不产生
    # 对 libgio/libglib 等共享库的 NEEDED 依赖，打包脚本也读取到缓存值0。
    if(IPC_CAP_BLUETOOTH)
        include(${IPC_PLATFORM_PATH}/lib/ble/ble.cmake)
    endif()

    # 人流统计新框架迁移能力
    # 组合规则：PIPELINE=0 => LEGACY（只运行旧处理器）；PIPELINE=1 => NEW（只运行新框架）
    if(NOT DEFINED IPC_CAP_AI_PEOPLE_FLOW_PIPELINE)
        message(FATAL_ERROR "IPC_CAP_AI_PEOPLE_FLOW_PIPELINE 未在设备画像 DEVICE_PROFILE_*_CMAKE_VARS 中定义，请检查 device_profiles.cmake")
    endif()
    if(NOT IPC_CAP_AI_PEOPLE_FLOW_PIPELINE EQUAL 0 AND NOT IPC_CAP_AI_PEOPLE_FLOW_PIPELINE EQUAL 1)
        message(FATAL_ERROR "IPC_CAP_AI_PEOPLE_FLOW_PIPELINE 取值非法: ${IPC_CAP_AI_PEOPLE_FLOW_PIPELINE}，只允许 0/1")
    endif()
    add_compile_definitions(CAP_AI_PEOPLE_FLOW_PIPELINE=${IPC_CAP_AI_PEOPLE_FLOW_PIPELINE})

    # 统一检测层新链路总开关
    # 语义：=0 全部算法走 Legacy（检测层不编译进固件）；=1 MD/OD/FACE 一并切换新链路并编译统一检测层
    if(NOT DEFINED IPC_CAP_UNIFIED_DETECTION)
        message(FATAL_ERROR "IPC_CAP_UNIFIED_DETECTION 未在设备画像 DEVICE_PROFILE_*_CMAKE_VARS 中定义，请检查 device_profiles.cmake")
    endif()
    if(NOT IPC_CAP_UNIFIED_DETECTION EQUAL 0 AND NOT IPC_CAP_UNIFIED_DETECTION EQUAL 1)
        message(FATAL_ERROR "IPC_CAP_UNIFIED_DETECTION 取值非法: ${IPC_CAP_UNIFIED_DETECTION}，只允许 0/1")
    endif()
    add_compile_definitions(CAP_UNIFIED_DETECTION=${IPC_CAP_UNIFIED_DETECTION})

    # 展馆人流统计自研模型
    # 语义：=0 人流统计走 HVF（现状）；=1 展馆 YOLO 模型接入统一检测层并编译关闭 HVF 人流统计
    # 两级门控：展馆代码仅在统一检测层总开关开启（IPC_CAP_UNIFIED_DETECTION=1）时编译
    if(NOT DEFINED IPC_CAP_AI_EXHIBITION_PEOPLE_FLOW)
        message(FATAL_ERROR "IPC_CAP_AI_EXHIBITION_PEOPLE_FLOW 未在设备画像 DEVICE_PROFILE_*_CMAKE_VARS 中定义，请检查 device_profiles.cmake")
    endif()
    if(NOT IPC_CAP_AI_EXHIBITION_PEOPLE_FLOW EQUAL 0 AND NOT IPC_CAP_AI_EXHIBITION_PEOPLE_FLOW EQUAL 1)
        message(FATAL_ERROR "IPC_CAP_AI_EXHIBITION_PEOPLE_FLOW 取值非法: ${IPC_CAP_AI_EXHIBITION_PEOPLE_FLOW}，只允许 0/1")
    endif()
    if(IPC_CAP_AI_EXHIBITION_PEOPLE_FLOW EQUAL 1 AND NOT IPC_CAP_UNIFIED_DETECTION EQUAL 1)
        message(FATAL_ERROR "IPC_CAP_AI_EXHIBITION_PEOPLE_FLOW=1 要求 IPC_CAP_UNIFIED_DETECTION=1，请检查设备画像配置")
    endif()
    add_compile_definitions(CAP_AI_EXHIBITION_PEOPLE_FLOW=${IPC_CAP_AI_EXHIBITION_PEOPLE_FLOW})

    # 人员密度新框架迁移
    # 语义：=0 人员密度走 HVF V2（现状）；=1 展馆 YOLO 模型 head 类接入共享 pipeline 并编译关闭 HVF 密度链路
    if(NOT DEFINED IPC_CAP_AI_PEOPLE_DENSITY_PIPELINE)
        message(FATAL_ERROR "IPC_CAP_AI_PEOPLE_DENSITY_PIPELINE 未在设备画像 DEVICE_PROFILE_*_CMAKE_VARS 中定义，请检查 device_profiles.cmake")
    endif()
    if(NOT IPC_CAP_AI_PEOPLE_DENSITY_PIPELINE EQUAL 0 AND NOT IPC_CAP_AI_PEOPLE_DENSITY_PIPELINE EQUAL 1)
        message(FATAL_ERROR "IPC_CAP_AI_PEOPLE_DENSITY_PIPELINE 取值非法: ${IPC_CAP_AI_PEOPLE_DENSITY_PIPELINE}，只允许 0/1")
    endif()
    if(IPC_CAP_AI_PEOPLE_DENSITY_PIPELINE EQUAL 1 AND NOT IPC_CAP_AI_EXHIBITION_PEOPLE_FLOW EQUAL 1)
        message(FATAL_ERROR "IPC_CAP_AI_PEOPLE_DENSITY_PIPELINE=1 要求 IPC_CAP_AI_EXHIBITION_PEOPLE_FLOW=1，请检查设备画像配置")
    endif()
    add_compile_definitions(CAP_AI_PEOPLE_DENSITY_PIPELINE=${IPC_CAP_AI_PEOPLE_DENSITY_PIPELINE})

    # 全局事件链路（新事件结构）总开关
    # 语义：=0 事件走旧事件结构或 per-event 宏控制；=1 事件走新事件结构（AiPipeline_NS）
    # 渐进替代：跨算法类的迁移总开关，最终替代 IPC_CAP_AI_PEOPLE_FLOW_PIPELINE 等 5 个 per-event 宏
    if(NOT DEFINED IPC_CAP_UNIFIED_EVENT_PIPELINE)
        message(FATAL_ERROR "IPC_CAP_UNIFIED_EVENT_PIPELINE 未在设备画像 DEVICE_PROFILE_*_CMAKE_VARS 中定义，请检查 device_profiles.cmake")
    endif()
    if(NOT IPC_CAP_UNIFIED_EVENT_PIPELINE EQUAL 0 AND NOT IPC_CAP_UNIFIED_EVENT_PIPELINE EQUAL 1)
        message(FATAL_ERROR "IPC_CAP_UNIFIED_EVENT_PIPELINE 取值非法: ${IPC_CAP_UNIFIED_EVENT_PIPELINE}，只允许 0/1")
    endif()
    add_compile_definitions(CAP_UNIFIED_EVENT_PIPELINE=${IPC_CAP_UNIFIED_EVENT_PIPELINE})
endmacro()

# macro（无独立作用域）：画像变量 set 到调用者作用域，末尾的校验可直接读取，
# add_compile_definitions 作用于调用者目录及以下，全局生效。
# 注意：macro 参数不是变量（function 才是），比较须用 "${device_type}" 展开
macro(apply_device_profile device_type)
    # 每次应用画像都先恢复蓝牙默认值，避免复用构建目录切换型号时继承
    # 上一个蓝牙型号写入 CMakeCache 的 IPC_CAP_BLUETOOTH=1。
    set(IPC_CAP_BLUETOOTH 0)
    set(_matched FALSE)
    foreach(_key IN LISTS DEVICE_PROFILE_KEYS)
        set(_expect_type "${DEVICE_PROFILE_${_key}_DEVICE_TYPE}")
        if("${device_type}" STREQUAL "${_expect_type}")
            set(_defines ${DEVICE_PROFILE_${_key}_DEFINES})
            add_compile_definitions(${_defines})

            foreach(_item IN LISTS DEVICE_PROFILE_${_key}_CMAKE_VARS)
                string(REPLACE "=" ";" _kv "${_item}")
                list(GET _kv 0 _name)
                list(GET _kv 1 _value)
                set(${_name} ${_value})
            endforeach()

            set(_matched TRUE)
            message(STATUS "DEVICE_PROFILE = ${_key}")
            break()
        endif()
    endforeach()

    if(NOT _matched)
        message(FATAL_ERROR "Unsupported DEVICE_TYPE: ${device_type}")
    endif()

    # 画像能力宏校验 + 编译下发（画像声明、校验、下发内聚本文件）
    validate_and_export_caps()
endmacro()
