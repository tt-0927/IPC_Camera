# classroom.cmake


unset(SOURCE_LIST)    # aux_source_directory 是追加语义，且 SOURCE_LIST 在 ai_app 链上跨文件共享，
                      # 不清零会把上游各 .cmake 的源文件再 append 一遍（现网靠 add_executable 去重侥幸）

# 头文件与源文件共用同一张目录表，保证声明范围与编译范围一致
# （model_conf 只放 JSON，故不入表）
set (INCLUDE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/common
    ${CMAKE_CURRENT_LIST_DIR}/conf
    ${CMAKE_CURRENT_LIST_DIR}/interface
    ${CMAKE_CURRENT_LIST_DIR}/bus
    ${CMAKE_CURRENT_LIST_DIR}/compute
    ${CMAKE_CURRENT_LIST_DIR}/perception
    ${CMAKE_CURRENT_LIST_DIR}/rules
    ${CMAKE_CURRENT_LIST_DIR}/behavior
    ${CMAKE_CURRENT_LIST_DIR}/data
    ${CMAKE_CURRENT_LIST_DIR}/egress
)

# 课堂配置下发接口（CAP_TV3881TJY_CLASSROOM_CONFIG）
set (CLASSROOM_CONFIG_TASK_PATH
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../share/ipc_share/control/task
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../share/ipc_share/data_format/json
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../share/ipc_share/shared_library/json/cJSON
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../share/ipc_share/common/define
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../share/ipc_share/common
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../share/ipc_share/design
)
foreach(item ${CLASSROOM_CONFIG_TASK_PATH})
    include_directories (${item})
endforeach()

foreach(item ${INCLUDE_PATH})
    include_directories (${item})
endforeach()

set (SOURCE_PATH ${INCLUDE_PATH})

foreach(item ${INCLUDE_PATH})
    aux_source_directory (${item} SOURCE_LIST)
endforeach()

list(APPEND SRC_LIST ${SOURCE_LIST})
