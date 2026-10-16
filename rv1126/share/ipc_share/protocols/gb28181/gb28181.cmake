# GB35114 国密能力门控：兜底与 CAP_GB35114 宏导出统一在 ipc_share.cmake 顶部
# （须先于 control.cmake 的 gm_cert 门控执行），此处只判不兜。
if(IPC_CAP_GB35114)
    include( ${CMAKE_CURRENT_LIST_DIR}/../../encrypt/gb35114/gb35114.cmake)
endif()

#源文件
set (GB28181_PATH
    ${CMAKE_CURRENT_LIST_DIR}/
    ${CMAKE_CURRENT_LIST_DIR}/sip
    ${CMAKE_CURRENT_LIST_DIR}/sip/common
    ${CMAKE_CURRENT_LIST_DIR}/sip/device
    ${CMAKE_CURRENT_LIST_DIR}/sip/event
    ${CMAKE_CURRENT_LIST_DIR}/sip/media
    ${CMAKE_CURRENT_LIST_DIR}/sip/media/libmpeg/source
    ${CMAKE_CURRENT_LIST_DIR}/sip/network
    ${CMAKE_CURRENT_LIST_DIR}/sip/request

)
# 国密签名源码目录随能力裁剪（关闭时不参与编译，SipClient 等调用点走 #if 分支）
if(IPC_CAP_GB35114)
    list(APPEND GB28181_PATH ${CMAKE_CURRENT_LIST_DIR}/sip/gm)
endif()
foreach(item ${GB28181_PATH})
    aux_source_directory (${item} GB28181_LIST)
endforeach()
list(REMOVE_ITEM GB28181_LIST ${CMAKE_CURRENT_LIST_DIR}/sip/common/ModuleLog.cpp)

#头文件
set (GB28181_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}/
    ${CMAKE_CURRENT_LIST_DIR}/../../../cam_share/common/log
    ${CMAKE_CURRENT_LIST_DIR}/sip
    ${CMAKE_CURRENT_LIST_DIR}/sip/common
    ${CMAKE_CURRENT_LIST_DIR}/sip/device
    ${CMAKE_CURRENT_LIST_DIR}/sip/event
    ${CMAKE_CURRENT_LIST_DIR}/sip/gm
    ${CMAKE_CURRENT_LIST_DIR}/sip/media
    ${CMAKE_CURRENT_LIST_DIR}/sip/media/libmpeg/include
    ${CMAKE_CURRENT_LIST_DIR}/sip/network
    ${CMAKE_CURRENT_LIST_DIR}/sip/request
)
foreach(item ${GB28181_INCLUDE})
    include_directories ( ${item} ) 
endforeach()
list(APPEND SRC_LIST ${GB28181_LIST})
