# -----------------------------------------------------------------------------
# CheckLinkerFlag 空实现垫片（CMake < 3.18 兼容）
#
# vendored libevent 的 cmake/AddLinkerFlags.cmake 文件头无条件
# include(CheckLinkerFlag)，而该模块 CMake 3.18 才内置：Ubuntu 20.04 自带的
# 3.16 等开发环境缺此模块，配置阶段直接报 include 找不到文件。上游对
# check_linker_flag() 的调用本身已有 VERSION_LESS 3.18 守卫，旧 CMake 下
# 不会真正执行。
#
# 根 CMakeLists.txt 仅在 CMAKE_VERSION < 3.18 时把本目录加入模块搜索路径，
# 让 include() 能找到模块；链接器 flag 检测退化为"不启用"。仅影响开发态
# SOURCE 模式的 libevent 构建，静态库产物不依赖这些链接器 flag；>= 3.18
# 优先命中内置模块，行为不变。vendor 源码不修改（third_party/README.md 约定）。
# -----------------------------------------------------------------------------
if(NOT COMMAND check_linker_flag)
    macro(check_linker_flag)
    endmacro()
endif()
