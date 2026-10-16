# ipc_share.cmake

# C++ 标准基线：共享层代码按 C++11 编写（富瀚 gcc 6.5 缺少 string_view 等
# C++17 库特性，按最低公分母）。上层工程已显式设置标准时（如 rv1126b_ipc
# 设 17）不覆盖，共享层源文件随上层标准编译；未设置的工程兜底 11
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 11)
endif()
# 编译器不支持目标标准时直接报错（而非降级）
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 禁用编译器专属扩展（如 GNU 的 -std=gnu++11）
set(CMAKE_CXX_EXTENSIONS OFF)

# mpark.variant：C++11 兼容的 variant 实现，替代 std::variant（C++17），
# DbBase、告警区域、ISP 配置、邮件参数等模块共用
include_directories(${CMAKE_CURRENT_LIST_DIR}/shared_library/mpark)

# GB35114 国密能力门控（设备画像 IPC_CAP_GB35114，0/1）。
# 兜底必须先于所有模块 include 执行（control.cmake 的 gm_cert 门控比
# protocols.cmake 的 gb35114 门控先跑），故统一放在本入口顶部。
# 未定义时默认开，保持 hi3516/rv1126b 现状；仅业务画像显式置 0 才裁剪。
if(NOT DEFINED IPC_CAP_GB35114)
    set(IPC_CAP_GB35114 1 CACHE INTERNAL "GB35114 国密能力，默认开（hi3516/rv1126b 现状）")
endif()

# 带值宏导出（if 外：关闭侧调用方 TU 也要有宏值，源码统一 #if CAP_GB35114 值判断）
add_compile_definitions(CAP_GB35114=${IPC_CAP_GB35114})

# 本地 SD 存储能力门控（设备画像 IPC_CAP_LOCAL_STORAGE，0/1）。
# 门控范围：录制/抓拍落盘/SD 卡管理/事件检索/回放任务/SD 数据库/record 进程通信。
# 兜底必须先于所有模块 include 执行（business.cmake 的片段门控在 include 链上），
# 故与 GB35114 一样统一放在本入口顶部。未定义时默认开，保持 hi3516/rv1126b 现状。
if(NOT DEFINED IPC_CAP_LOCAL_STORAGE)
    set(IPC_CAP_LOCAL_STORAGE 1 CACHE INTERNAL "本地 SD 存储，默认开（hi3516/rv1126b 现状）")
endif()

# 带值宏导出（if 外：关闭侧调用方 TU 也要有宏值，源码统一 #if CAP_LOCAL_STORAGE 值判断）
add_compile_definitions(CAP_LOCAL_STORAGE=${IPC_CAP_LOCAL_STORAGE})

# 音频输出/对讲能力门控（设备画像 IPC_CAP_AUDIO_OUTPUT，0/1）。
# 门控范围：AO 播放回调管道、web/GB28181/TVSDK 对讲与广播下行、声音联动播放、
# rtp/ 音频收流目录；音频输入（拾音→AENC→推流）与 TVSDK 对讲上行不受此宏影响。
# 兜底必须先于所有模块 include 执行（network.cmake 内 rtp.cmake 的目录门控在
# include 链上），故与 GB35114/LOCAL_STORAGE 一样统一放在本入口顶部。
# 未定义时默认开，保持 hi3516/rv1126b 现状。
if(NOT DEFINED IPC_CAP_AUDIO_OUTPUT)
    set(IPC_CAP_AUDIO_OUTPUT 1 CACHE INTERNAL "音频输出/对讲，默认开（hi3516/rv1126b 现状）")
endif()

# 带值宏导出（if 外：关闭侧调用方 TU 也要有宏值，源码统一 #if CAP_AUDIO_OUTPUT 值判断）
add_compile_definitions(CAP_AUDIO_OUTPUT=${IPC_CAP_AUDIO_OUTPUT})

include(${CMAKE_CURRENT_LIST_DIR}/bestshot/bestshot.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/common/common.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/control/control.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/data_format/data_format.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/design/design.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/encrypt/md5/md5.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/ffmpeg/ffmpeg.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/hardware/hardware.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/network/network.cmake)
# include(${CMAKE_CURRENT_LIST_DIR}/network/tcp/tcp.cmake)
# include(${CMAKE_CURRENT_LIST_DIR}/network/udp/udp.cmake)
# include(${CMAKE_CURRENT_LIST_DIR}/network/websocket/websocket.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/protocols/protocols.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/push_stream/push_stream.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/ret/ret.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/standard/standard.cmake)
