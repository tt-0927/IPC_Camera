# OSD 字体后端二选一（对齐 IPC_RTSP_BACKEND / IPC_WS_BACKEND 机制）：
#   dotfont  点阵字库后端（目标后端；ipc_osd_16.bin + dotfont.c，零第三方库依赖）
#   sdl      SDL2 + SDL2_ttf + FreeType + simhei.ttf 后端（现状，回退保留）
# 两个后端在 overplay_draw.cpp / overplay_draw.h 内按宏切换实现，对外 OSD 能力一致
# （单行/多行文本测量渲染、矩形绘制、ARGB4444 直出 RGN）。
# 平台库（SDL2.cmake / SDL2_ttf.cmake / freetype.cmake）的引入与链接由取值方按需条件化。
if(NOT DEFINED IPC_OSD_FONT_BACKEND)
    set(IPC_OSD_FONT_BACKEND "sdl" CACHE STRING "OSD 字体后端：dotfont（新）| sdl（旧）")
endif()

if(IPC_OSD_FONT_BACKEND STREQUAL "dotfont")
    add_compile_definitions(IPC_OSD_FONT_BACKEND_DOTFONT)
    set(OSD_FONT_SRC_PATH
        ${CMAKE_CURRENT_LIST_DIR}/dotfont/
    )
elseif(IPC_OSD_FONT_BACKEND STREQUAL "sdl")
    set(OSD_FONT_SRC_PATH
        ${CMAKE_CURRENT_LIST_DIR}/sdl/
    )
else()
    message(FATAL_ERROR
        "IPC_OSD_FONT_BACKEND 必须是 dotfont 或 sdl，当前为：${IPC_OSD_FONT_BACKEND}")
endif()

foreach(item ${OSD_FONT_SRC_PATH})
    include_directories ( ${item} )
    aux_source_directory (${item} OSD_FONT_SRC_LIST)
endforeach()

list(APPEND SRC_LIST ${OSD_FONT_SRC_LIST} )
