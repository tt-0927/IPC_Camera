if(ENABLE_GAT1400)
    include(${CMAKE_CURRENT_LIST_DIR}/viid/viid.cmake)
    message(STATUS "==== GAT1400 support ====")
    # 使能GAT1400代码相关宏定义
    add_compile_definitions(ENABLE_GAT1400_SRC)

    set (GAT1400_PATH
        ${CMAKE_CURRENT_LIST_DIR}/
    )
    #源文件
    foreach(item ${GAT1400_PATH})
        aux_source_directory (${item} GAT1400_LIST)
    endforeach()
    #头文件
    set (GAT1400_INCLUDE
        ${CMAKE_CURRENT_LIST_DIR}/
    )
    foreach(item ${GAT1400_INCLUDE})
        include_directories ( ${item} ) 
    endforeach()

    list(APPEND SRC_LIST ${GAT1400_LIST})
else()
    message(STATUS "==== GAT1400 not support ====")
endif()