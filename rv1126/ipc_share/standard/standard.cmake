include( ${CMAKE_CURRENT_LIST_DIR}/thread/thread.cmake )
include( ${CMAKE_CURRENT_LIST_DIR}/cond/cond.cmake )
include( ${CMAKE_CURRENT_LIST_DIR}/mutex/mutex.cmake )
include( ${CMAKE_CURRENT_LIST_DIR}/sem/sem.cmake )
include( ${CMAKE_CURRENT_LIST_DIR}/thrdpool/thrdpool.cmake )
include( ${CMAKE_CURRENT_LIST_DIR}/ringBuf/ringBuf.cmake )
include( ${CMAKE_CURRENT_LIST_DIR}/queue/queue.cmake )
include( ${CMAKE_CURRENT_LIST_DIR}/debug/debug.cmake )
include( ${CMAKE_CURRENT_LIST_DIR}/kernel/kernel.cmake )
include( ${CMAKE_CURRENT_LIST_DIR}/atom/atom.cmake )
include( ${CMAKE_CURRENT_LIST_DIR}/hashCode/hashCode.cmake )
# include( ${CMAKE_CURRENT_LIST_DIR}/diagnostics/thread_performance.cmake )

#源文件
set (OS_PATH
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${OS_PATH})
    aux_source_directory (${item} OS_LIST)
endforeach()
#头文件
set (OS_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${OS_INCLUDE})
    include_directories ( ${item} ) 
endforeach()

list(APPEND SRC_LIST ${OS_LIST})
