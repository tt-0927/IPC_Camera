set(INFERENCE_NAME "Retinaface")

# 必须在前面
add_library(${INFERENCE_NAME} INTERFACE)

# 清除变量
unset(MODULE_SOURCES)

# 文件  
set (SRC_PATH
    # 后处理依赖
    ${CMAKE_CURRENT_LIST_DIR}/../../../
    
    ${CMAKE_CURRENT_LIST_DIR}/../../../PostProcess/
    ${CMAKE_CURRENT_LIST_DIR}/../../Common/Tokenizer/
    # 芯片推理依赖
    ${CMAKE_CURRENT_LIST_DIR}/../../
    ${CMAKE_CURRENT_LIST_DIR}/../../Common/ModelOpt/
)
foreach(item ${SRC_PATH})
    target_include_directories(${INFERENCE_NAME} INTERFACE ${item})
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
    target_include_directories(${INFERENCE_NAME} INTERFACE ${item})
    aux_source_directory (${item} MODULE_SOURCES)
endforeach() 

# 设置的源文件
target_sources(${INFERENCE_NAME} INTERFACE ${MODULE_SOURCES})

# 链接库路径
target_link_libraries(${INFERENCE_NAME} INTERFACE 
    librockit
)

message("==========> [" ${PROJECT_NAME} "] 使用[" ${INFERENCE_NAME} "]模块 <==========")