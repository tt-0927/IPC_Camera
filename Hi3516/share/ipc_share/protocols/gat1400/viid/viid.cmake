if(ENABLE_GAT1400)
    message(STATUS "==== VIID support ====")
    # 需要支持ssl
    add_compile_definitions(CPPHTTPLIB_OPENSSL_SUPPORT)

    set (VIID_PATH
        ${CMAKE_CURRENT_LIST_DIR}/
    )

    foreach(item ${VIID_PATH})
        aux_source_directory (${item} VIID_LIST)
    endforeach()
    #头文件
    set (VIID_INCLUDE
        ${CMAKE_CURRENT_LIST_DIR}/
        ${CMAKE_CURRENT_LIST_DIR}/json/
    )
    foreach(item ${VIID_INCLUDE})
        include_directories ( ${item} ) 
    endforeach()

    list(APPEND SRC_LIST ${VIID_LIST})
else()
    message(STATUS "==== VIID not support ====")
endif()