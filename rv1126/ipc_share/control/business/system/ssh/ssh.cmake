# ssh.cmake

set (INCLUDE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
)

set (SOURCE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
)

foreach(item ${INCLUDE_PATH})
    include_directories (${item})
endforeach()

foreach(item ${SOURCE_PATH})
    aux_source_directory (${item} SOURCE_LIST)
endforeach()

list(APPEND SRC_LIST ${SOURCE_LIST})
