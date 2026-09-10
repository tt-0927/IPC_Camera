# 源文件
set (THREAD_PERFORMANCE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${THREAD_PERFORMANCE_PATH})
    aux_source_directory (${item} THREAD_PERFORMANCE_LIST)
endforeach()

# 头文件
set (THREAD_PERFORMANCE_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${THREAD_PERFORMANCE_INCLUDE})
    include_directories (${item})
endforeach()

list(APPEND SRC_LIST ${THREAD_PERFORMANCE_LIST})
