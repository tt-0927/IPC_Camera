# common.cmake

# 引用
include(${CMAKE_CURRENT_LIST_DIR}/database/database.cmake)

# 头文件
set (INCLUDE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/event_manager
    ${CMAKE_CURRENT_LIST_DIR}/image_processor
    ${CMAKE_CURRENT_LIST_DIR}/sdk_event
)

if(IPC_CAP_EXHIBITION_OSD_PANEL)
    list(APPEND INCLUDE_PATH
        # 展会版左上角面板通用结构
        ${CMAKE_CURRENT_LIST_DIR}/osd_panel
    )
endif()

# 源文件
set (SOURCE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/event_manager
    ${CMAKE_CURRENT_LIST_DIR}/image_processor
    ${CMAKE_CURRENT_LIST_DIR}/sdk_event
)

if(IPC_CAP_EXHIBITION_OSD_PANEL)
    list(APPEND SOURCE_PATH
        # 展会版左上角面板通用结构实现
        ${CMAKE_CURRENT_LIST_DIR}/osd_panel
    )
endif()

# 添加头文件
foreach(item ${INCLUDE_PATH})
    include_directories (${item}) 
endforeach()

# 添加源文件
foreach(item ${SOURCE_PATH})
    aux_source_directory (${item} SOURCE_LIST)
endforeach()

list(APPEND SRC_LIST ${SOURCE_LIST})
