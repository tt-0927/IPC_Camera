#头文件
set (BLOCKINGQUEUE_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}
)
foreach(item ${BLOCKINGQUEUE_INCLUDE})
    include_directories ( ${item} ) 
endforeach()
