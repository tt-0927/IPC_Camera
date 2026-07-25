# ai.cmake

# 引用

if(DEVICE_TYPE STREQUAL "TV-3882TI")
include(${CMAKE_CURRENT_LIST_DIR}/analysis_mode/analysis_mode.cmake)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/detect_mode/detect_mode.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/common/common.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/interface/interface.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/stream_process/stream_process.cmake)


# 头文件
set (INCLUDE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
)

# 源文件
set (SOURCE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
)

# 添加头文件
foreach(item ${INCLUDE_PATH})
    include_directories (${item}) 
endforeach()

# 添加源文件
foreach(item ${SOURCE_PATH})
    aux_source_directory (${item} SOURCE_LIST)
endforeach()

list(APPEND SRC_LIST ${SOURCE_LIST})