# 外设通用设备实现。
# 该目录仅依赖 GPIO/PWM 基础设施和稳定画像，不读取任何产品能力宏。
set (PERIPHERAL_INCLUDE_PATH
    ${CMAKE_CURRENT_LIST_DIR}/fill_light
    ${CMAKE_CURRENT_LIST_DIR}/ircut
)

set (PERIPHERAL_SOURCE_PATH
    ${CMAKE_CURRENT_LIST_DIR}/fill_light
    ${CMAKE_CURRENT_LIST_DIR}/ircut
)

foreach(item ${PERIPHERAL_INCLUDE_PATH})
    include_directories (${item})
endforeach()

set (PERIPHERAL_SOURCE_LIST)
foreach(item ${PERIPHERAL_SOURCE_PATH})
    aux_source_directory (${item} PERIPHERAL_SOURCE_TEMP)
    list(APPEND PERIPHERAL_SOURCE_LIST ${PERIPHERAL_SOURCE_TEMP})
    unset(PERIPHERAL_SOURCE_TEMP)
endforeach()

list(APPEND SRC_LIST ${PERIPHERAL_SOURCE_LIST})
