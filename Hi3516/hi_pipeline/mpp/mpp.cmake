# mpp.cmake

# 引用cmake
include(${CMAKE_CURRENT_LIST_DIR}/adec/adec.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/aenc/aenc.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/ai/ai.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/ao/ao.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/common/common.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/resample/resample.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/rgn/rgn.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/venc/venc.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/vgs/vgs.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/vi/vi.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/vpss/vpss.cmake)

# 头文件
set (INCLUDE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/cbb/audio/adp/include
    ${CMAKE_CURRENT_LIST_DIR}/cbb/isp/include
    ${CMAKE_CURRENT_LIST_DIR}/cbb/isp/user/3a/include
    ${CMAKE_CURRENT_LIST_DIR}/cbb/isp/user/sensor/hi3516cv610/common
    ${CMAKE_CURRENT_LIST_DIR}/cbb/isp/user/sensor/hi3516cv610/smart_sc500ai
    ${CMAKE_CURRENT_LIST_DIR}/cbb/isp/user/sensor/hi3516cv610/smart_sc533hai
)

# 源文件
set (SOURCE_PATH
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/cbb/audio/adp/src
    ${CMAKE_CURRENT_LIST_DIR}/cbb/isp/user/sensor/hi3516cv610/common
    ${CMAKE_CURRENT_LIST_DIR}/cbb/isp/user/sensor/hi3516cv610/smart_sc500ai
    ${CMAKE_CURRENT_LIST_DIR}/cbb/isp/user/sensor/hi3516cv610/smart_sc533hai
)

# 添加源文件
foreach(item ${SOURCE_PATH})
aux_source_directory (${item} SOURCE_LIST)
endforeach()

# 排除特定源文件
list(FILTER SOURCE_LIST EXCLUDE REGEX "ot_audio_mp3_adp\\.c$")
list(FILTER SOURCE_LIST EXCLUDE REGEX "ot_audio_opus_adp\\.c$")

list(FILTER INCLUDE_PATH EXCLUDE REGEX "ot_audio_mp3_adp\\.h$")
list(FILTER INCLUDE_PATH EXCLUDE REGEX "ot_audio_opus_adp\\.h$")

message(${SOURCE_LIST})
# 添加头文件
foreach(item ${INCLUDE_PATH})
    include_directories (${item}) 
endforeach()

list(APPEND HI_PIPELINE_LIST ${SOURCE_LIST})