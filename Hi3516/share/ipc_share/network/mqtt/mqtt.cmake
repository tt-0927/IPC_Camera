# mqtt.cmake

include(${CAM_SHARE_PATH}/communication/mqtt/mqtt.cmake)

#源文件
set (MQTT_PATH
    ${CMAKE_CURRENT_LIST_DIR}
)

foreach(item ${MQTT_PATH})
    aux_source_directory (${item} MQTT_LIST)
endforeach()

#头文件
set (MQTT_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}
)

foreach(item ${MQTT_INCLUDE})
    include_directories ( ${item} ) 
endforeach()

list(APPEND SRC_LIST ${MQTT_LIST})
