# ai.cmake

# 引用
include(${CMAKE_CURRENT_LIST_DIR}/detect_mode/detect_mode.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/common/common.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/interface/interface.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/stream_process/stream_process.cmake)
# 课堂 AI 分析：引擎源码与配置下发接口共用同一开关，未开启的型号不入编译集
if(IPC_CAP_CLASSROOM)
    include(${CMAKE_CURRENT_LIST_DIR}/detect_mode/classroom/classroom.cmake)
endif()


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