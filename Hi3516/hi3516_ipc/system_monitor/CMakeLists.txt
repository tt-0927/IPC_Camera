# 版本限定
cmake_minimum_required(VERSION 3.12)

# 设置程序名称
SET(PRO_NAME "system_monitor")
project(${PRO_NAME})

SET(LIBS
	-lstdc++
	-lpthread
)
set (LOCAL_LIBS
)

# 过滤
set(GREP_TEXT "demo")

# 获取本程序子目录
execute_process(
    COMMAND find ${CMAKE_CURRENT_LIST_DIR}/  -type d
    COMMAND grep -vE ${GREP_TEXT} # 过滤目录 U
    OUTPUT_VARIABLE dirs_list
    )
string(REPLACE "\n" ";" dirs_list ${dirs_list})

# 添加所有源文件与头文件
foreach(item ${dirs_list})
    include_directories (${item})
    aux_source_directory (${item} SRC_LIST)
endforeach()

# 编译
add_executable(${PROJECT_NAME}
 			   ${SRC_LIST} )

# 链接库路径
target_link_libraries(${PROJECT_NAME} ${LIBS} ${LOCAL_LIBS})

message("==========> 生成 [" ${PROJECT_NAME} "] Makefile文件 成功 <==========")
