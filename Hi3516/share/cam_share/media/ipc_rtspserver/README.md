# ipc_rtspserver

`share/cam_share/media/ipc_rtspserver/` 是面向嵌入式 IPC 的轻量 RTSP Server 库。
协议核心为固定版本的 [OpenIPC/smolrtsp](https://github.com/OpenIPC/smolrtsp)（C99），
事件后端为固定版本的
[OpenIPC/libevent](https://github.com/OpenIPC/libevent) fork（pin `694decef`，首期静态链接）。
本库为 C++17，承担**连接管理、会话、鉴权、RTP/RTCP、媒体分发与背压**全部职责。

libevent 支持两种接入方式：主机/CI 使用 `SOURCE` 模式，产品构建使用 `SDK` 模式。
第三方源码和本库都归属于 `cam_share`，不复制到 `share/ipc_share`；产品薄封装仍在
`share/ipc_share/push_stream/rtsp_smol/`（同名 `CRtspServer`，调用方零改动）。

设计与兼容基线见仓库根目录的 `docs/smolrtsp_rtspserver_docs/19~23`。

## 目录结构

```text
ipc_rtspserver/
├── CMakeLists.txt          库目标 ipc_rtspserver（默认静态库）
├── cmake/                  构建选项、smolrtsp 离线接线、libevent 构建接线
├── include/ipc_rtsp/       对外公共头（唯一对外接口）
├── src/                    实现
│   ├── adapter/            smolrtsp 隔离层（唯一包含 smolrtsp 头文件的 C99 单元）
│   ├── net/                libevent 事件循环 / 监听 / 连接 / 有界输出 / UDP
│   ├── rtsp/               控制器 / 会话 / URL 路由 / SDP
│   ├── auth/               Digest 认证
│   ├── media/              FrameHub / Annex-B 扫描 / 参数集缓存 / 邮箱 / 文件源
│   ├── rtp/                每 track RTP 发送器与 RTCP SR/BYE
│   ├── metrics/            计数器
│   └── support/            日志 / 时间 / MD5 / 文本
├── poc/                    主机验证程序（离线 Annex-B 文件模拟 VENC）
├── tests/                  单元测试（ctest）
└── tools/                  协议探针与主机验证脚本
```

## 构建与验证（主机）

主机源码模式默认使用仓库内的过渡镜像（`third_party/{smolrtsp,libevent}`），**无需联网**。
在部门第三方源码工作树中开发时，可直接覆盖 libevent 路径，不需要复制或打包：

```bash
cmake -S media/ipc_rtspserver -B build/rtsp_host \
    -DIPC_RTSP_LIBEVENT_MODE=SOURCE \
    -DIPC_RTSP_LIBEVENT_ROOT=/path/to/cam_opensource_library/libevent-openipc/libevent-2.2.2-openipc \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

产品模式由 `ipc_platform/lib/libevent/` 直接导入平台目录中的静态归档：

```text
ipc_platform/lib/libevent/
├── include/event2/       # 与目标归档匹配的头文件和 event-config.h
└── lib/libevent.a        # OpenIPC libevent，Hi3516 已带 BROKEN_MMAP
```

该目录由 `cam_opensource_library/libevent-openipc` 构建结果同步生成，业务侧不需要再给
`build.sh` 传 libevent 参数。

产品侧链接**必须带 `-Wl,--gc-sections`**（本库以 INTERFACE 方式带出该选项）——
文档中的体积数字都以此为前提。

```bash
# 1) 构建库 + PoC + 单测（源码模式）
cmake --build build/rtsp_host -j"$(nproc)"

# 可选：校验第三方 SDK 的 pin、target 和核心归档
./media/ipc_rtspserver/tools/verify_libevent_sdk.sh \
    /path/to/libevent-openipc/output/<target> <target>

# 2) 单元测试
ctest --test-dir build/rtsp_host --output-on-failure

# 3) 端到端主机验证（准备码流 → 起 PoC → 协议探针 → ffprobe/ffmpeg 拉流 → RTP 校验）
./media/ipc_rtspserver/tools/host_verify.sh              # 无鉴权
./media/ipc_rtspserver/tools/host_verify.sh --auth       # 额外验证 Digest 鉴权
```

单独跑 PoC：

```bash
./build/rtsp_host/poc/rtspserver_poc --port 8554 \
    --main /tmp/rtsp_media/main.h264 --sub /tmp/rtsp_media/sub.h265 --fps 25 --verbose 1

ffplay -rtsp_transport tcp rtsp://127.0.0.1:8554/Streaming/Channels/101
ffplay -rtsp_transport udp rtsp://127.0.0.1:8554/Streaming/Channels/102
python3 media/ipc_rtspserver/tools/rtsp_probe.py --host 127.0.0.1 --port 8554 --suite all \
    --user admin --password zfrl@168
```

## 关键约束（改代码前先读）

- smolrtsp 及其 99 宏依赖在 `third_party/smolrtsp/` 固定版本；本地修改必须登记在
  `third_party/smolrtsp/PATCHES.md`，不能静默改 vendor 源码。
- libevent 的 source-of-truth 在 `cam_opensource_library`；`third_party/libevent/` 只作为
  `cam_share` 内的迁移期离线镜像，不能产生第二份独立修改。
- 业务代码不得包含 smolrtsp 头文件；所有协议交互经 `src/adapter/smolrtsp_shim.h`。
- 任何 queue/buffer 必须先回答：最大条数、最大字节、满时策略（见 21 文档）。
- 一个 pack = 一个 Access Unit；marker 仅在 AU 最后一个 NAL。
- 单 I/O 线程；生产者只入有界邮箱；慢客户端不得影响编码线程与其他客户端。
- 性能/内存结论必须板端实测（`tools/rtsp_mem_sample.sh` + `rtsp_mem_summary.py`）。

## 构建选项

| 选项 | 默认 | 说明 |
|---|---|---|
| `IPC_RTSP_BACKEND` | `smolrtsp` | 产品后端；`live555` 仅用于显式回滚 |
| `IPC_RTSP_SHARED` | OFF | 构建共享库而非静态库 |
| `IPC_RTSP_LIBEVENT_MODE` | `SOURCE` | libevent 接入模式：`SOURCE` 或 `SDK` |
| `IPC_RTSP_LIBEVENT_ROOT` | `third_party/libevent` | `SOURCE` 模式的 libevent 源码根目录 |
| `IPC_RTSP_LIBEVENT_SDK_DIR` | 空 | `SDK` 模式的 staging 根目录 |
| `IPC_RTSP_LIBEVENT_BROKEN_MMAP` | OFF | 对 libevent 编译目标定义 `BROKEN_MMAP` |
| `IPC_RTSP_EXCEPTIONS` | OFF | 库内启用 exception/RTTI（默认关闭） |
| `IPC_RTSP_WITH_MJPEG` | OFF | 打开 smolrtsp MJPEG payload（RFC 2435） |
| `IPC_RTSP_BUILD_TESTS` | ON | 构建主机侧单元测试 |
| `IPC_RTSP_BUILD_POC` | ON | 构建主机验证程序 |
| `IPC_RTSP_VENDOR_ROOT` | `third_party/smolrtsp` | vendored smolrtsp 家族根目录 |
| `IPC_RTSP_WARNINGS_AS_ERRORS` | OFF | 告警即错误 |

## 产品集成（父工程）

```cmake
# hi3516_ipc/ipc.cmake 已默认设置为 smolrtsp；需要回滚时显式改为 live555。
set(IPC_RTSP_BACKEND "smolrtsp" CACHE STRING "RTSP 后端：smolrtsp | live555")
# ipc.cmake 导入 ipc_platform/lib/rtsp_smol/lib/ 下的 RTSP 与 smolrtsp 静态归档；
# ipc_share.cmake → rtsp_smol.cmake 只收集 CRtspServer 薄封装源码。
target_link_libraries(<业务目标> PRIVATE ${IPC_RTSP_LINK_LIBS})
```

`IPC_RTSP_BACKEND` 默认 `smolrtsp`；业务代码不需要改动。需要回滚时显式设置
`-DIPC_RTSP_BACKEND=live555`，此时才接入旧 `libRtspServer.so`。

## 当前状态（2026-09-11，事件后端已切到 libevent）

已验证：主机 ctest **8 组**全过（含 `test_event_loop`）；协议探针 TCP/UDP/负向/Digest
全过；ffprobe（TCP+UDP）与 ffmpeg 解码通过；RTP 头字段校验（序号连续、同 AU 同时间戳、
marker 每 AU 仅一个、单 PT/SSRC）通过；ARM 交叉构建通过。

体积（ARM musl、`-Os`、`--gc-sections`、stripped，同一 PoC 前后对比）：
自研 epoll 112,524 B → libevent **145,424 B**（+32.9 KB，+29%）；
作为交换，`event_base` 可直接承接后续 Web 控制通道的 `evhttp`/`evws`，
并可从 rootfs 删除 `libwebsockets.so.19`（249,228 B，详见 22 文档 D2-R1）。

待办：板端 VENC 接入与内存/弱网/多客户端压测（S3–S4）、音频端到端、
RTP-Info（如需）、Digest-SHA256。父工程 `stream` 已默认切换 smolrtsp，并从
`ipc_platform/lib/libevent/lib/libevent.a` 静态链接。详见 23 文档第 6 节。
