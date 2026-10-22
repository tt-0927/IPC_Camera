# libevent 平台静态库

本目录按平台库的现有布局提供 RTSP 第一阶段所需的 OpenIPC libevent：

```text
include/event2/      # 与目标工具链匹配的头文件和 event-config.h
lib/libevent.a       # core 静态归档，Hi3516 构建时已启用 BROKEN_MMAP
```

产物来源：

```text
同一工作区的 `camera/cam_opensource_library/libevent-openipc/`
```

更新归档时，先使用 `libevent-openipc/compile.sh` 交叉编译，再同步其
`build/include/` 和 `build/lib/libevent*.a` 到本目录。不要使用 stock libevent，
也不要直接修改生成的 `event-config.h`。
