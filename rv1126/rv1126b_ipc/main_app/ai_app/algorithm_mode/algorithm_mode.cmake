# algorithm_mode.cmake

# 纯算法组件目录（不继承 CAlgorithm，不接取流，由调用方驱动）

# 头文件
set (INCLUDE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/channel_occupation_detect_model
    ${CMAKE_CURRENT_LIST_DIR}/climb_detect_model
    ${CMAKE_CURRENT_LIST_DIR}/common
    ${CMAKE_CURRENT_LIST_DIR}/face_attribute_model
    ${CMAKE_CURRENT_LIST_DIR}/face_detect_model
    ${CMAKE_CURRENT_LIST_DIR}/face_expression
    ${CMAKE_CURRENT_LIST_DIR}/face_quality_model
    ${CMAKE_CURRENT_LIST_DIR}/fire_person_detect
    ${CMAKE_CURRENT_LIST_DIR}/mouse_detect
    ${CMAKE_CURRENT_LIST_DIR}/kitchen_detect
    ${CMAKE_CURRENT_LIST_DIR}/pmnm_detect
    ${CMAKE_CURRENT_LIST_DIR}/fight_detect
    ${CMAKE_CURRENT_LIST_DIR}/group4_detect
    ${CMAKE_CURRENT_LIST_DIR}/tracker
)

# 源文件
set (SOURCE_PATH
    ${CMAKE_CURRENT_LIST_DIR}/channel_occupation_detect_model
    ${CMAKE_CURRENT_LIST_DIR}/climb_detect_model
    ${CMAKE_CURRENT_LIST_DIR}/face_attribute_model
    ${CMAKE_CURRENT_LIST_DIR}/face_detect_model
    ${CMAKE_CURRENT_LIST_DIR}/face_expression
    ${CMAKE_CURRENT_LIST_DIR}/face_quality_model
    ${CMAKE_CURRENT_LIST_DIR}/fire_person_detect
    ${CMAKE_CURRENT_LIST_DIR}/mouse_detect
    ${CMAKE_CURRENT_LIST_DIR}/kitchen_detect
    ${CMAKE_CURRENT_LIST_DIR}/pmnm_detect
    ${CMAKE_CURRENT_LIST_DIR}/fight_detect
    ${CMAKE_CURRENT_LIST_DIR}/group4_detect
    ${CMAKE_CURRENT_LIST_DIR}/tracker
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
