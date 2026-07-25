#源文件
set (SRC_PATH
    ${CMAKE_CURRENT_LIST_DIR}/
    ${CMAKE_CURRENT_LIST_DIR}/inference_record
)
foreach(item ${SRC_PATH})
    include_directories ( ${item} ) 
    aux_source_directory (${item} LLM_SRC_LIST)
endforeach()

list(APPEND SRC_LIST ${LLM_SRC_LIST} )