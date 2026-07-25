# 过滤
set(GREP_TEXT "demo|Demo")

# 获取ai_share-0630应用目录
execute_process(
    COMMAND find ${CMAKE_CURRENT_LIST_DIR} -type d 
    COMMAND grep -vE ${GREP_TEXT} # 过滤目录 U
    OUTPUT_VARIABLE dirs_list
    )
string(REPLACE "\n" ";" dirs_list ${dirs_list})
# 添加所有源文件与头文件
foreach(item ${dirs_list})
    include_directories (${item})
    aux_source_directory (${item} SRC_0630AI_LIST)
endforeach()

list(APPEND SRC_LIST ${SRC_0630AI_LIST} )