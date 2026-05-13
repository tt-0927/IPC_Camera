#SRC
set(SRC_PATH
    ${CMAKE_CURRENT_LIST_DIR}/code
)


foreach(item ${SRC_PATH})
    include_directories(${item})
    aux_source_directory(${item} SECUREC_SRC_LIST)
endforeach()

list(APPEND SRC_LIST ${SECUREC_SRC_LIST})
