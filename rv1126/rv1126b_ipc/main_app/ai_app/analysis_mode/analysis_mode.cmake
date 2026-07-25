# analysis_mode.cmake

#RKVLM
include(${AI_SHARE_PATH}/AiModules/Inference/RKVLM/RKVLM.cmake)

# 引用
include(${CMAKE_CURRENT_LIST_DIR}/visionText_analysis/visionText_analysis.cmake)

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
