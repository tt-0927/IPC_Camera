# business.cmake

include(${CMAKE_CURRENT_LIST_DIR}/av/av.cmake)
# 抓拍落盘/本地录制/SD 卡管理属于本地 SD 存储链路，随 IPC_CAP_LOCAL_STORAGE 裁剪
# （兜底在 ipc_share.cmake 顶部；宏关闭时对应源文件与头文件路径一起退出 SRC_LIST）
if(IPC_CAP_LOCAL_STORAGE)
    include(${CMAKE_CURRENT_LIST_DIR}/capture/capture.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/record/record.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/storage/storage.cmake)
endif()
include(${CMAKE_CURRENT_LIST_DIR}/event/event.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/log/log.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/network/network.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/system/system.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/system/gpioCtrl/gpioCtrl.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/preview/preview.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/peripheral/peripheral.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/isp/isp.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/osd/osd.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/ai_student_business/ai_student_business.cmake)
