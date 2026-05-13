#源文件

set (SRC_PATH
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../../shared_library/json
    ${CMAKE_CURRENT_LIST_DIR}/../../shared_library/json/cJSON
    ${CMAKE_CURRENT_LIST_DIR}/../../shared_library/json/rapidjson
    ${CMAKE_CURRENT_LIST_DIR}/../../shared_library/mpark/
)
foreach(item ${SRC_PATH})
    include_directories ( ${item} ) 
    aux_source_directory (${item} JSON_SRC_LIST)
endforeach()

list(APPEND SRC_LIST ${JSON_SRC_LIST} )