set(MODULE_NAME "ClipText_Module")

# 必须在前面
add_library(${MODULE_NAME} INTERFACE)

# include(${CMAKE_CURRENT_LIST_DIR}/../../Inference/RK/Classification/TextFeature/TextFeature.cmake)


# 清除变量
unset(MODULE_SOURCES)

# 文件  
set (SRC_PATH
    ${CMAKE_CURRENT_LIST_DIR}/../Common/
)
foreach(item ${SRC_PATH})
    target_include_directories(${MODULE_NAME} INTERFACE ${item})
    aux_source_directory(${item} MODULE_SOURCES)
endforeach()

# 过滤
set(GREP_TEXT "demo|Demo")

# 获取本程序子目录
execute_process(
    COMMAND find ${CMAKE_CURRENT_LIST_DIR}/  -type d 
    COMMAND grep -vE ${GREP_TEXT} # 过滤目录 U
    OUTPUT_VARIABLE dirs_list
    )
string(REPLACE "\n" ";" dirs_list ${dirs_list})
# 添加所有源文件与头文件
foreach(item ${dirs_list})
    target_include_directories(${MODULE_NAME} INTERFACE ${item})
    aux_source_directory (${item} MODULE_SOURCES)
endforeach() 

# 设置的源文件
target_sources(${MODULE_NAME} INTERFACE ${MODULE_SOURCES})

# 链接库路径
target_link_libraries(${MODULE_NAME} INTERFACE 
    TextFeature
)

message("==========> [" ${PROJECT_NAME} "] 使用[" ${MODULE_NAME} "]模块 <==========")