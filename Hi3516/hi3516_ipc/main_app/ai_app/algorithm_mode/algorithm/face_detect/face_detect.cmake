set(FACE_DETECT_ROOT ${CMAKE_CURRENT_LIST_DIR})

execute_process(
    COMMAND find ${FACE_DETECT_ROOT} -type d
    OUTPUT_VARIABLE FACE_DETECT_DIRS
)

string(REPLACE "\n" ";" FACE_DETECT_DIRS "${FACE_DETECT_DIRS}")

unset(FACE_DETECT_SOURCE_LIST)

foreach(item ${FACE_DETECT_DIRS})
    if (NOT item STREQUAL "")
        include_directories(${item})
        aux_source_directory(${item} FACE_DETECT_SOURCE_LIST)
    endif()
endforeach()

list(APPEND SOURCE_LIST ${FACE_DETECT_SOURCE_LIST})
