include( ${CMAKE_CURRENT_LIST_DIR}/rtsp/rtsp.cmake )
include( ${CMAKE_CURRENT_LIST_DIR}/rtmp/rtmp.cmake )

#源文件
set (PUSH_STREAM_PATH
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${PUSH_STREAM_PATH})
    aux_source_directory (${item} PUSH_STREAM_LIST)
endforeach()
#头文件
set (PUSH_STREAM_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${PUSH_STREAM_INCLUDE})
    include_directories ( ${item} ) 
endforeach()
list(APPEND SRC_LIST ${PUSH_STREAM_LIST})
