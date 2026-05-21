# TVSDK 编译宏：用于在业务代码中条件编译 TVSDK 相关逻辑（如告警推送）
add_compile_definitions(ENABLE_TVSDK_SRC)


include(${CMAKE_CURRENT_LIST_DIR}/src/tvsdk_src.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/include/tvsdk_inc.cmake)
