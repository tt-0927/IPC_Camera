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
#   使用: share/ipc_share/control/business/system/pwmCtrl/pwm_ctrl.h,
#         share/ipc_share/control/business/system/pwmCtrl/light_manager.cpp
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
# - CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE: record 需要额外 cam_share include
#   使用: hi3516_ipc/ipc.cmake, share/ipc_share/public_app/record/CMakeLists.txt
# - CAP_RECORD_LINK_FDK_AAC: record 需要链接 fdk-aac
#   使用: hi3516_ipc/ipc.cmake, share/ipc_share/public_app/record/CMakeLists.txt
# - CAP_AI_GARBAGE_DETECT: 垃圾暴露/垃圾满溢检测能力
#   使用: hi3516_ipc/main_app/ai_app/algorithm_mode/algorithm/garbage_detect/garbage_detect.hpp,
#         hi3516_ipc/main_app/ai_app/interface/algo_stream_deal.cpp,
#         share/ipc_share/common/define/event_define.h
# - CAP_AI_PEOPLE_STATISTICS: 人流统计/人员密度检测能力
#   使用: share/ipc_share/common/define/alarm_define.h,
#         share/ipc_share/common/define/event_define.h,
#         share/ipc_share/control/business/event,
#         share/ipc_share/control/task/sub_task/event_task.cpp
# - CAP_EXHIBITION_OSD_PANEL: 展会版左上角 AI 汇总面板能力，并拦截网页 OSD 设置
#   使用: hi3516_ipc/main_app/ai_app/common/common_process.cpp,
#         hi3516_ipc/main_app/stream_media/video/osd/osd_manage.cpp,
#         hi3516_ipc/main_app/stream_media/video/osd/overplay_draw.cpp,
#         share/ipc_share/control/task/sub_task/picture_task.cpp
# - CAP_AI_USE_SIMPLE_JSON: AI 使用 simple Json 头
#   使用: share/ai_share/AiModules/Inference/Hisilicon/CVInferenceHISI.hpp
# - CAP_ISP_DAY_NIGHT_DIFFERENT_TUNING: ISP 日夜差异化调参能力（WDR/曝光补偿/锐度）
#   使用: hi3516_ipc/main_app/stream_media/video/isp/isp_control.h,
#         hi3516_ipc/main_app/stream_media/video/isp/isp_control.cpp
#   说明: 仅 TV-3852H* 系列启用；TV-3852T* 系列白天黑夜都走白天参数逻辑
# - CAP_ISP_SCENE_LIGHT_PARAM: ISP 白光场景参数能力
#   使用: hi3516_ipc/main_app/stream_media/video/isp/isp_scene.cpp,
#         share/ipc_share/control/business/isp/isp_manage.cpp
#   说明: 仅 TV-3852H* 系列启用；夜间白光模式使用独立 light 场景参数
# - CAP_ISP_CMOS_GAMMA_3852H: TV-3852H* 系列 CMOS Gamma 曲线能力
#   使用: hi_pipeline/mpp/cbb/isp/user/sensor/hi3516cv610/smart_sc533hai/sc533hai_cmos_param.h,
#         hi_pipeline/mpp/cbb/isp/user/sensor/sensor_foundation/sensor_template/template_sns_cmos_param.h
#   说明: 仅 TV-3852H* 系列启用；TV-3852T* 系列保持原 Gamma 曲线
# - CAP_NETWORK_4G: 4G 网络能力
#   使用: （预留，暂未实现）
# - CAP_NETWORK_WIFI: WIFI 网络能力
#   使用: （预留，暂未实现）
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

set(DEVICE_PROFILE_KEYS
    TV_3852T
    TV_3852H
    TV_3852TL
    TV_3852HL
    TV_3852TL4G
    TV_3852TLW
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
    CAP_LIGHT_WHITE_ONLY=0               # 仅白光灯能力（pwm_ctrl.h / light_manager.cpp）
    CAP_PWM_NEED_POLARITY=0              # PWM 极性配置（pwm_utils.cpp）
    CAP_EVENT_AUDIO_PLAYBACK_V2=0        # 事件音频播放 V2（event_linkage.cpp）
    CAP_AUDIO_PLAYBACK_SLEEP_HALF=1      # 音频播放 half-sleep（event_linkage.cpp）
    CAP_SYSTEM_REBOOT_MUTE=0             # 系统重启静音处理（system_manage.cpp / event_resource.cpp）
    CAP_STORAGE_MMCBLK1=0                # 存储 mmcblk1 路径逻辑（storage_manage.cpp）
    CAP_PROCESS_USE_PS_GREP=0            # 进程探测 ps|grep（process_manager.cpp）
    CAP_PROCESS_KILL_CROND_BEFORE_STREAM=0  # 拉流前处理 crond（process_manager.cpp）
    CAP_DAEMON_LOG_BY_SIZE=1             # 守护进程按大小滚动日志（daemon/main.cpp）
    CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE=0 # record 额外 include（ipc.cmake / record/CMakeLists.txt）
    CAP_RECORD_LINK_FDK_AAC=0            # record 链接 fdk-aac（ipc.cmake / record/CMakeLists.txt）
    CAP_AI_GARBAGE_DETECT=0              # 垃圾暴露/垃圾满溢检测能力（garbage_detect.hpp / algo_stream_deal.cpp / event_define.h）
    CAP_AI_PEOPLE_STATISTICS=0           # 人流统计/人员密度检测能力
    CAP_EXHIBITION_OSD_PANEL=0           # 展会版AI左上角汇总面板能力
    CAP_AI_USE_SIMPLE_JSON=1             # AI 使用 simple Json 头（CVInferenceHISI.hpp）
    CAP_GPIO_IR_CUT_JSON=1               # gpio控制ir_cut方式（gpio_ctrl.cpp）
    CAP_ISP_SCENE_LIGHT_PARAM=0          # 独立白光场景参数能力（TV-3852H* 专用）
    CAP_ISP_CMOS_GAMMA_3852H=0           # TV-3852H* CMOS Gamma 曲线能力
    CAP_NETWORK_TELNET_SERVICE=0         # telnet 服务能力；发布版本不启动 telnetd
    CAP_NETWORK_FTP_SERVICE=0            # ftp 服务能力；发布版本不启动 uftpd
)
set(DEVICE_PROFILE_TV_3852T_CMAKE_VARS
    IPC_CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE=0 # record 额外 include（ipc.cmake / record/CMakeLists.txt）
    IPC_CAP_RECORD_LINK_FDK_AAC=0            # record 链接 fdk-aac（ipc.cmake / record/CMakeLists.txt）
    IPC_CAP_EXHIBITION_OSD_PANEL=0           # 展会版AI左上角汇总面板能力
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
    CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE=0
    CAP_RECORD_LINK_FDK_AAC=0
    CAP_AI_GARBAGE_DETECT=0
    CAP_AI_PEOPLE_STATISTICS=0
    CAP_AI_USE_SIMPLE_JSON=1
    CAP_GPIO_IR_CUT_JSON=0
    CAP_ISP_DAY_NIGHT_DIFFERENT_TUNING=1 # ISP 日夜差异化调参开启；仅 TV-3852H* 系列生效
    CAP_ISP_SCENE_LIGHT_PARAM=1          # 夜间白光模式使用独立 light 场景参数
    CAP_ISP_CMOS_GAMMA_3852H=1           # TV-3852H* 使用独立 CMOS Gamma 曲线
    CAP_NETWORK_TELNET_SERVICE=1
    CAP_NETWORK_FTP_SERVICE=1
)
set(DEVICE_PROFILE_TV_3852H_CMAKE_VARS
    IPC_CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE=0
    IPC_CAP_RECORD_LINK_FDK_AAC=0
)

# ========== 智能垃圾站系列型号 ==========

set(DEVICE_PROFILE_TV_3852TL_DEVICE_TYPE "TV-3852TL")
# TV-3852TL 设备能力画像（智能垃圾站筒型海螺型有线）
# - 继承 852T 基线
# - 差异点：开启垃圾检测能力，有线网络
set(DEVICE_PROFILE_TV_3852TL_DEFINES
    DEVICE_TV_3852TL
    CAP_ALARM_IO=1
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
    CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE=0
    CAP_RECORD_LINK_FDK_AAC=0
    CAP_AI_GARBAGE_DETECT=1              # 垃圾暴露/垃圾满溢检测能力
    CAP_AI_PEOPLE_STATISTICS=0           # 人流统计/人员密度检测能力
    CAP_AI_USE_SIMPLE_JSON=1
    CAP_GPIO_IR_CUT_JSON=1
    CAP_NETWORK_TELNET_SERVICE=0         # telnet 服务能力；发布版本不启动 telnetd
    CAP_NETWORK_FTP_SERVICE=0            # ftp 服务能力；发布版本不启动 uftpd
    CAP_NETWORK_4G=0                     # 4G网络（预留）
    CAP_NETWORK_WIFI=0                   # WIFI网络（预留）
)
set(DEVICE_PROFILE_TV_3852TL_CMAKE_VARS
    IPC_CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE=0
    IPC_CAP_RECORD_LINK_FDK_AAC=0
    IPC_CAP_EXHIBITION_OSD_PANEL=0           # 展会版AI左上角汇总面板能力
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
    CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE=0
    CAP_RECORD_LINK_FDK_AAC=0
    CAP_AI_GARBAGE_DETECT=1              # 垃圾暴露/垃圾满溢检测能力
    CAP_AI_PEOPLE_STATISTICS=0           # 人流统计/人员密度检测能力
    CAP_AI_USE_SIMPLE_JSON=1
    CAP_NETWORK_4G=0                     # 4G网络（预留）
    CAP_NETWORK_WIFI=0                   # WIFI网络（预留）
    CAP_ISP_DAY_NIGHT_DIFFERENT_TUNING=1
    CAP_ISP_SCENE_LIGHT_PARAM=1
    CAP_ISP_CMOS_GAMMA_3852H=1
)
set(DEVICE_PROFILE_TV_3852HL_CMAKE_VARS
    IPC_CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE=0
    IPC_CAP_RECORD_LINK_FDK_AAC=0
)

set(DEVICE_PROFILE_TV_3852TL4G_DEVICE_TYPE "TV-3852TL4G")
# TV-3852TL4G 设备能力画像（智能垃圾站筒型海螺型4G）
# - 继承 852T 基线
# - 差异点：开启垃圾检测能力，4G网络能力宏预留
set(DEVICE_PROFILE_TV_3852TL4G_DEFINES
    DEVICE_TV_3852TL4G
    CAP_ALARM_IO=1
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
    CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE=0
    CAP_RECORD_LINK_FDK_AAC=0
    CAP_AI_GARBAGE_DETECT=1              # 垃圾暴露/垃圾满溢检测能力
    CAP_AI_PEOPLE_STATISTICS=0           # 人流统计/人员密度检测能力
    CAP_AI_USE_SIMPLE_JSON=1
    CAP_GPIO_IR_CUT_JSON=1
    CAP_NETWORK_TELNET_SERVICE=0         # telnet 服务能力；发布版本不启动 telnetd
    CAP_NETWORK_FTP_SERVICE=0            # ftp 服务能力；发布版本不启动 uftpd
    CAP_NETWORK_4G=1                     # 4G网络（预留）
    CAP_NETWORK_WIFI=0                   # WIFI网络（预留）
)
set(DEVICE_PROFILE_TV_3852TL4G_CMAKE_VARS
    IPC_CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE=0
    IPC_CAP_RECORD_LINK_FDK_AAC=0
    IPC_CAP_EXHIBITION_OSD_PANEL=0           # 展会版AI左上角汇总面板能力
)

set(DEVICE_PROFILE_TV_3852TLW_DEVICE_TYPE "TV-3852TLW")
# TV-3852TLW 设备能力画像（智能垃圾站筒型海螺型WIFI）
# - 继承 852T 基线
# - 差异点：开启垃圾检测能力，WIFI网络能力宏预留
set(DEVICE_PROFILE_TV_3852TLW_DEFINES
    DEVICE_TV_3852TLW
    CAP_ALARM_IO=1
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
    CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE=0
    CAP_RECORD_LINK_FDK_AAC=0
    CAP_AI_GARBAGE_DETECT=1              # 垃圾暴露/垃圾满溢检测能力
    CAP_AI_PEOPLE_STATISTICS=0           # 人流统计/人员密度检测能力
    CAP_AI_USE_SIMPLE_JSON=1
    CAP_GPIO_IR_CUT_JSON=1
    CAP_NETWORK_TELNET_SERVICE=0         # telnet 服务能力；发布版本不启动 telnetd
    CAP_NETWORK_FTP_SERVICE=0            # ftp 服务能力；发布版本不启动 uftpd
    CAP_NETWORK_4G=0                     # 4G网络（预留）
    CAP_NETWORK_WIFI=1                   # WIFI网络（预留）
)
set(DEVICE_PROFILE_TV_3852TLW_CMAKE_VARS
    IPC_CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE=0
    IPC_CAP_RECORD_LINK_FDK_AAC=0
    IPC_CAP_EXHIBITION_OSD_PANEL=0           # 展会版AI左上角汇总面板能力
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
