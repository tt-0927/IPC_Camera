set(HVF_DETECT_ROOT ${CMAKE_CURRENT_LIST_DIR})

execute_process(
    COMMAND find ${HVF_DETECT_ROOT} -type d
    OUTPUT_VARIABLE HVF_DETECT_DIRS
)

string(REPLACE "\n" ";" HVF_DETECT_DIRS "${HVF_DETECT_DIRS}")

unset(HVF_DETECT_SOURCE_LIST)

foreach(item ${HVF_DETECT_DIRS})
    if (NOT item STREQUAL "")
        include_directories(${item})
        aux_source_directory(${item} HVF_DETECT_SOURCE_LIST)
    endif()
endforeach()

list(APPEND SRC_LIST ${HVF_DETECT_SOURCE_LIST})
