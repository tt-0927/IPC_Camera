# codec_audio.cmake

include( ${CMAKE_CURRENT_LIST_DIR}/g711/codec_g711.cmake)
# include( ${CMAKE_CURRENT_LIST_DIR}/mp2/codec_mp2.cmake)

#源文件
set (CODEC_PATH
    ${CMAKE_CURRENT_LIST_DIR}
)

foreach(item ${CODEC_PATH})
    aux_source_directory (${item} CODEC_LIST)
endforeach()

#头文件
set (CODEC_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}
)

foreach(item ${CODEC_INCLUDE})
    include_directories ( ${item} ) 
endforeach()

list(APPEND SRC_LIST ${CODEC_LIST})
