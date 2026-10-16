#源文件
set (SRC_PATH
    ${CMAKE_CURRENT_LIST_DIR}/
    ${CMAKE_CURRENT_LIST_DIR}/../
)
foreach(item ${SRC_PATH})
    include_directories ( ${item} )
    # rtp/ 目录只含 rtp_audio_receiver（对讲/广播音频 UDP 收流，纯音频输出模块），
    # 随 IPC_CAP_AUDIO_OUTPUT 门控；network 顶层（edukit_network.c）保持无条件收集
    if(IPC_CAP_AUDIO_OUTPUT OR item STREQUAL "${CMAKE_CURRENT_LIST_DIR}/../")
        aux_source_directory (${item} RTP_SRC_LIST)
    endif()
endforeach()

list(APPEND SRC_LIST ${RTP_SRC_LIST} )