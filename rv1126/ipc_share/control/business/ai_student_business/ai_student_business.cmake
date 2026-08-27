# ai_student_business.cmake
if(ENABLE_AI_STUDENT)
    message(STATUS "==== AI student business support ====")
    add_compile_definitions(ENABLE_AI_STUDENT)
    # 头文件
    set (INCLUDE_PATH
        ${CMAKE_CURRENT_LIST_DIR}
    )

    # 源文件
    set (SOURCE_PATH
        ${CMAKE_CURRENT_LIST_DIR}
    )

    # 添加头文件
    foreach(item ${INCLUDE_PATH})
        include_directories (${item})
    endforeach()

    # 添加源文件
    foreach(item ${SOURCE_PATH})
        aux_source_directory (${item} SOURCE_LIST)
    endforeach()

    list(APPEND SRC_LIST ${SOURCE_LIST})
else()
    message(STATUS "==== AI student business not support ====")
endif()