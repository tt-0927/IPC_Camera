# smolrtsp RTSP 平台静态库

本目录提供 `stream` 产品使用的预编译 RTSP 静态库：

```text
include/ipc_rtsp/        # cam_share/media/ipc_rtspserver/ 公共头文件
lib/libipc_rtspserver.a  # cam_share/media/ipc_rtspserver/ 的 C++ RTSP 实现
lib/libsmolrtsp.a        # OpenIPC smolrtsp C99 协议核心
```

`hi3516_ipc` 通过本目录的 `rtsp_smol.cmake` 导入这两个归档；
`share/ipc_share/push_stream/rtsp_smol/` 只编译产品薄封装，
不再编译 `cam_share/media/ipc_rtspserver/`。

## 归档更新

归档由 `cam_opensource_library/libIpcRtspServer/` 统一交叉编译发布:

```bash
cd /path/to/cam_opensource_library/libIpcRtspServer
./compile.sh /opt/hisi-linux/x86-arm/arm-v01c02-linux-musleabi-gcc/bin/arm-v01c02-linux-musleabi- \
    --sync-ipc-platform /path/to/ipc_rtsp_server/ipc_platform/lib/rtsp_smol
```

脚本只同步 `libipc_rtspserver.a`、`include/ipc_rtsp/` 与 `build_info.txt`;
`libsmolrtsp.a` 维持本目录的 vendored 基线并做 SHA-256 校验,不同步。
完整发布流程(源码打包、host 门禁、重建 stream)见
`docs/smolrtsp_rtspserver_docs/27_发布流程_库归档与stream更新.md`。

`libevent.a` 仍由 `ipc_platform/lib/libevent/` 单独提供(来源
`cam_opensource_library/libevent-openipc/`);三者必须使用同一目标
工具链和 smolrtsp 基线，不能混用其他平台归档。
