# web_plugin.cmake

# 网页插件工具模块的头文件目录
set(WEB_PLUGIN_UTILS_INCLUDE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
)

# 网页插件工具模块的源文件目录
set(WEB_PLUGIN_UTILS_SOURCE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
)

foreach(item ${WEB_PLUGIN_UTILS_INCLUDE_PATH})
    include_directories(${item})
endforeach()

foreach(item ${WEB_PLUGIN_UTILS_SOURCE_PATH})
    aux_source_directory(${item} WEB_PLUGIN_UTILS_SOURCE_LIST)
endforeach()

list(APPEND SRC_LIST ${WEB_PLUGIN_UTILS_SOURCE_LIST})
