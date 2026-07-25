# rk_pipeline.cmake

include(${CMAKE_CURRENT_LIST_DIR}/include/include.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/3rdparty/3rdparty.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/adec/adec.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/aenc/aenc.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/ai/ai.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/ao/ao.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/common/common.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/rgn/rgn.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/tde/tde.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/venc/venc.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/vi/vi.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/vpss/vpss.cmake)


link_directories(${CMAKE_CURRENT_LIST_DIR}/lib)