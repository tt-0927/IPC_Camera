# detect_mode.cmake

# 头文件
set (INCLUDE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/motion_detect
    ${CMAKE_CURRENT_LIST_DIR}/hide_detect
    ${CMAKE_CURRENT_LIST_DIR}/object_detect
    ${CMAKE_CURRENT_LIST_DIR}/face_detect
    ${CMAKE_CURRENT_LIST_DIR}/scene_change_detect
    ${CMAKE_CURRENT_LIST_DIR}/pet_recognition
    ${CMAKE_CURRENT_LIST_DIR}/audio_detect
    ${CMAKE_CURRENT_LIST_DIR}/group1_detect
    ${CMAKE_CURRENT_LIST_DIR}/group2_group4_detect
    ${CMAKE_CURRENT_LIST_DIR}/group3_detect
    ${CMAKE_CURRENT_LIST_DIR}/group5_detect
    # ${CMAKE_CURRENT_LIST_DIR}/sleep_detect
    # ${CMAKE_CURRENT_LIST_DIR}/trip_detect
    # ${CMAKE_CURRENT_LIST_DIR}/licensePlateCognition_detect
    # ${CMAKE_CURRENT_LIST_DIR}/phoneUsage_detect
    # ${CMAKE_CURRENT_LIST_DIR}/person_detect
    # ${CMAKE_CURRENT_LIST_DIR}/fire_detect
    # ${CMAKE_CURRENT_LIST_DIR}/stree_detect
    # ${CMAKE_CURRENT_LIST_DIR}/rubish_detect
    # ${CMAKE_CURRENT_LIST_DIR}/constructionEncroachmentRoad_detect
    # ${CMAKE_CURRENT_LIST_DIR}/highSafyBelt_Soil_detect
    # ${CMAKE_CURRENT_LIST_DIR}/helmet_detect 
    # ${CMAKE_CURRENT_LIST_DIR}/reflective_detect
    # ${CMAKE_CURRENT_LIST_DIR}/smoking_detect
    # ${CMAKE_CURRENT_LIST_DIR}/fence_detect
    # ${CMAKE_CURRENT_LIST_DIR}/pmnm_detect
    # ${CMAKE_CURRENT_LIST_DIR}/electricScooter_detect
    # ${CMAKE_CURRENT_LIST_DIR}/vehicle_detect
)

# 源文件
set (SOURCE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/motion_detect
    ${CMAKE_CURRENT_LIST_DIR}/hide_detect
    ${CMAKE_CURRENT_LIST_DIR}/object_detect
    ${CMAKE_CURRENT_LIST_DIR}/face_detect
    ${CMAKE_CURRENT_LIST_DIR}/scene_change_detect
    ${CMAKE_CURRENT_LIST_DIR}/pet_recognition
    ${CMAKE_CURRENT_LIST_DIR}/audio_detect
    ${CMAKE_CURRENT_LIST_DIR}/group1_detect
    ${CMAKE_CURRENT_LIST_DIR}/group2_group4_detect
    ${CMAKE_CURRENT_LIST_DIR}/group3_detect
    ${CMAKE_CURRENT_LIST_DIR}/group5_detect
    # ${CMAKE_CURRENT_LIST_DIR}/sleep_detect
    # ${CMAKE_CURRENT_LIST_DIR}/trip_detect
    # ${CMAKE_CURRENT_LIST_DIR}/phoneUsage_detect
    # ${CMAKE_CURRENT_LIST_DIR}/person_detect
    # ${CMAKE_CURRENT_LIST_DIR}/constructionEncroachmentRoad_detect
    # ${CMAKE_CURRENT_LIST_DIR}/highSafyBelt_Soil_detect
    # ${CMAKE_CURRENT_LIST_DIR}/helmet_detect
    # ${CMAKE_CURRENT_LIST_DIR}/reflective_detect
    # ${CMAKE_CURRENT_LIST_DIR}/smoking_detect
    # ${CMAKE_CURRENT_LIST_DIR}/fence_detect
    # ${CMAKE_CURRENT_LIST_DIR}/fire_detect
    # ${CMAKE_CURRENT_LIST_DIR}/stree_detect
    # ${CMAKE_CURRENT_LIST_DIR}/rubish_detect
    # ${CMAKE_CURRENT_LIST_DIR}/pmnm_detect
    # ${CMAKE_CURRENT_LIST_DIR}/electricScooter_detect
    # ${CMAKE_CURRENT_LIST_DIR}/vehicle_detect
    # ${CMAKE_CURRENT_LIST_DIR}/licensePlateCognition_detect
)

# 添加头文件
foreach(item ${INCLUDE_PATH})
    include_directories (${item}) 
endforeach()

# 添加源文件
foreach(item ${SOURCE_PATH})
    aux_source_directory (${item} SOURCE_LIST)
endforeach()

# 启用共享库的json文件
add_definitions(-DSHAREJSON_ENABLE)

list(APPEND SRC_LIST ${SOURCE_LIST})