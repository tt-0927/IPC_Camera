#源文件

set (SRC_PATH
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/sqlite3
    ${CMAKE_CURRENT_LIST_DIR}/../../shared_library/mpark
    # ${CMAKE_CURRENT_LIST_DIR}/../../shared_library/database
)
foreach(item ${SRC_PATH})
    include_directories ( ${item} ) 
    aux_source_directory (${item} DATABASE_SRC_LIST)
endforeach()

list(APPEND SRC_LIST ${DATABASE_SRC_LIST} )