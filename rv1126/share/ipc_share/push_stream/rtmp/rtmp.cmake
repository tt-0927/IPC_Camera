# RTMP推流模块CMake配置

set(RTMP_PATH
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/audio
    ${CMAKE_CURRENT_LIST_DIR}/flv
    ${CMAKE_CURRENT_LIST_DIR}/session
    ${CMAKE_CURRENT_LIST_DIR}/video
)

# 源文件
foreach(item ${RTMP_PATH})
    include_directories ( ${item} )
    aux_source_directory ( ${item} RTMP_SRC_LIST)
endforeach()

list(APPEND SRC_LIST ${RTMP_SRC_LIST})
