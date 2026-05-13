# algorithm.cmake

include(${CMAKE_CURRENT_LIST_DIR}/YoloUltralytics.cmake)
add_all_yolo_modules()

# 头文件
set (INCLUDE_PATH
${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/audio_detect
    ${CMAKE_CURRENT_LIST_DIR}/face_detect
    ${CMAKE_CURRENT_LIST_DIR}/garbage_detect
    ${CMAKE_CURRENT_LIST_DIR}/hide_detect
    ${CMAKE_CURRENT_LIST_DIR}/item_detect
    ${CMAKE_CURRENT_LIST_DIR}/motion_detect
    # ${CMAKE_CURRENT_LIST_DIR}/parking_detect
    ${CMAKE_CURRENT_LIST_DIR}/scene_change_detect
    ${CMAKE_CURRENT_LIST_DIR}/pet_recognition
    ${CMAKE_CURRENT_LIST_DIR}/people_head_detect
)

# 源文件
set (SOURCE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/audio_detect
    ${CMAKE_CURRENT_LIST_DIR}/garbage_detect
    ${CMAKE_CURRENT_LIST_DIR}/hide_detect
    ${CMAKE_CURRENT_LIST_DIR}/item_detect
    ${CMAKE_CURRENT_LIST_DIR}/motion_detect
    # ${CMAKE_CURRENT_LIST_DIR}/parking_detect
    ${CMAKE_CURRENT_LIST_DIR}/scene_change_detect
    ${CMAKE_CURRENT_LIST_DIR}/pet_recognition
)

# 添加头文件
foreach(item ${INCLUDE_PATH})
    include_directories (${item}) 
endforeach()

# 添加源文件
foreach(item ${SOURCE_PATH})
    aux_source_directory (${item} SOURCE_LIST)
endforeach()

include(${CMAKE_CURRENT_LIST_DIR}/people_head_detect/people_head_detect.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/hvf_detect/hvf_detect.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/face_detect/face_detect.cmake)

list(APPEND SRC_LIST ${SOURCE_LIST})
