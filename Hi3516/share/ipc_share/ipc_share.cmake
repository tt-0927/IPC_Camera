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
