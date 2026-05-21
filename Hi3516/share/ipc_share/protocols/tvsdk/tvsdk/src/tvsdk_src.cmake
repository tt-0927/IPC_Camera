

set(TVSDK_DIR     ${CMAKE_CURRENT_LIST_DIR})
set(TVSDK_LIB_DIR ${TVSDK_DIR}/../include)

function(recursive_include dir)
    include_directories(${dir})
    file(GLOB_RECURSE subdirs "${dir}/**")
    foreach(subdir ${subdirs})
        if(IS_DIRECTORY ${subdir})
            include_directories(${subdir})
        endif()
    endforeach()
endfunction()

# 本目录头文件
recursive_include(${TVSDK_DIR})

# include_directories(${TVSDK_DIR})
# lib 中的头文件（如 NetTVSDKServer.h）
include_directories(${TVSDK_LIB_DIR})

file(GLOB_RECURSE TVSDK_SRC
    "${TVSDK_DIR}/*.cpp"
    "${TVSDK_DIR}/*.c"
)
list(APPEND SRC_LIST ${TVSDK_SRC})

