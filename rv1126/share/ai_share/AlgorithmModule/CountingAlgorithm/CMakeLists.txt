# 版本限定
cmake_minimum_required(VERSION 3.10.2)

set(MODULE_NAME "CountingAlgorithm")

# 必须在前面
add_library(${MODULE_NAME} ${MODULE_SOURCES})

set(CODE_PATH 
    ${CMAKE_CURRENT_LIST_DIR}/../CommonModule/DataQueue
    ${CMAKE_CURRENT_LIST_DIR}/../CommonModule/ImageProcessor
)

# 添加所有源文件与头文件
foreach(item ${CODE_PATH})
    target_include_directories(${MODULE_NAME} PUBLIC ${item})
    aux_source_directory (${item} MODULE_SOURCES)
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
    target_include_directories(${MODULE_NAME} PUBLIC ${item})
    aux_source_directory (${item} MODULE_SOURCES)
endforeach() 

# 获取算法路径
set(ALGORITHM_PATH 
    ${CMAKE_CURRENT_LIST_DIR}/../../AiAlgorithm/BaseAIModel/Segmentation/RK_HumanCount
    ${CMAKE_CURRENT_LIST_DIR}/../../AiAlgorithm/BaseAIModel/TargetDetection/RK_HeadDetect
)
execute_process(
    COMMAND find ${ALGORITHM_PATH}/  -type d 
    COMMAND grep -vE ${GREP_TEXT} # 过滤目录 U
    OUTPUT_VARIABLE dirs_list_algorithm
)
string(REPLACE "\n" ";" dirs_list_algorithm ${dirs_list_algorithm})
# 添加算法源文件与头文件
foreach(item ${dirs_list_algorithm})
    target_include_directories(${MODULE_NAME} PUBLIC ${item})
    aux_source_directory (${item} MODULE_SOURCES)
endforeach()

# 设置的源文件
target_sources(${MODULE_NAME} PRIVATE ${MODULE_SOURCES})


message("==========> [" ${PROJECT_NAME} "] 使用[" ${MODULE_NAME} "]模块 <==========")
