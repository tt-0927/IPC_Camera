set(PEOPLE_HEAD_DETECT_ROOT ${CMAKE_CURRENT_LIST_DIR})

execute_process(
    COMMAND find ${PEOPLE_HEAD_DETECT_ROOT} -type d
    OUTPUT_VARIABLE PEOPLE_HEAD_DETECT_DIRS
)

string(REPLACE "\n" ";" PEOPLE_HEAD_DETECT_DIRS "${PEOPLE_HEAD_DETECT_DIRS}")

unset(PEOPLE_HEAD_DETECT_SOURCE_LIST)

foreach(item ${PEOPLE_HEAD_DETECT_DIRS})
    if (NOT item STREQUAL "")
        include_directories(${item})
        aux_source_directory(${item} PEOPLE_HEAD_DETECT_SOURCE_LIST)
    endif()
endforeach()

list(APPEND SOURCE_LIST ${PEOPLE_HEAD_DETECT_SOURCE_LIST})
