# static_first.cmake
#
# ipc_platform/lib 下各平台库共用的选库逻辑：优先链接静态库（lib*.a），
# 同名静态库缺失的库才回退链接动态库（lib*.so*）。
# SONAME 带版本后缀的动态库（如 libSDL2-2.0.so.0）会再去掉 -<数字> 后缀
# 探测一次对应的静态归档（libSDL2.a）。
#
# 用法（在各库的 .cmake 中）：
#   include("${CMAKE_CURRENT_LIST_DIR}/../static_first.cmake")
#   ipc_platform_static_first(GMSSL_LIBRARY "${GMSSL_LIBRARY_DIR}")

include_guard(GLOBAL)

function(ipc_platform_static_first out_var lib_dir)
    file(GLOB _sfl_static "${lib_dir}/lib*.a")
    file(GLOB _sfl_shared "${lib_dir}/lib*.so*")
    set(_sfl_picked "")
    foreach(_sfl_so IN LISTS _sfl_shared)
        get_filename_component(_sfl_name "${_sfl_so}" NAME)
        # 从 lib<库名>.so[.版本] 提取库名，用于探测同名静态库
        string(REGEX REPLACE "^lib(.+)\\.so(\\.[0-9]+)*$" "\\1" _sfl_stem "${_sfl_name}")
        if(NOT EXISTS "${lib_dir}/lib${_sfl_stem}.a" AND _sfl_stem MATCHES "^(.+)-[0-9.]+$")
            string(REGEX REPLACE "^(.+)-[0-9.]+$" "\\1" _sfl_base "${_sfl_stem}")
            if(EXISTS "${lib_dir}/lib${_sfl_base}.a")
                set(_sfl_stem "${_sfl_base}")
            endif()
        endif()
        if(NOT EXISTS "${lib_dir}/lib${_sfl_stem}.a")
            list(APPEND _sfl_picked "${_sfl_so}")
        endif()
    endforeach()
    # 静态库排在链接序列最前。工具链默认 --as-needed + 归档单遍扫描，
    # 库之间的先后顺序由消费方的 target_link_libraries 负责，
    # 有排序要求的库（xml2/osip2/snmp++ 等）见各自 cmake 的注释
    set(${out_var} ${_sfl_static} ${_sfl_picked} PARENT_SCOPE)
endfunction()
