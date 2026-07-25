# IPC_CAMERA RTSP / live555 深度接手指南

> 适用范围：当前 `E:\Code\IPC_Camera` 工作区中的 Hi3516 IPC 工程。
>
> 阅读目标：不仅知道“RTSP 能播放”，还要能沿着源码回答以下问题：编码帧从哪里来、何时入队、live555 何时取帧、RTSP/SDP/RTP/RTCP 分别负责什么、为什么新客户端必须请求 IDR、UDP 与 TCP 如何切换、音频怎样封装、重启或多客户端时可能在哪里出问题。

## 1. 先给出项目中的准确定位

本项目的 RTSP 不是 FFmpeg 推流，也不是把媒体主动推送到某个远端 RTSP 平台。

它是一个运行在 IPC 设备内部的 RTSP Server：

1. 海思 MPP 的 VENC/AENC 产生编码后的 H.264、H.265、MJPEG、AAC、G.711 或 G.726 数据。
2. IPC 业务层把编码数据放入主码流、子码流各自的 RTSP 队列。
3. 客户端通过 RTSP 连接设备，例如 VLC、ffplay、NVR 或 ONVIF 客户端。
4. live555 解析 RTSP 请求，生成 SDP，并按客户端选择的 UDP 或 TCP 方式发送 RTP/RTCP。
5. live555 需要数据时，通过项目注册的 `rtspFrameCall()` 回调从队列取数据。

因此它的方向是：

```text
IPC 编码器 -> 设备内 RTSP Server -> 网络客户端
```

而不是：

```text
IPC -> FFmpeg 命令行 -> 远端 RTSP Server
```

主、子码流 URL 为：

```text
主码流：rtsp://<ip>:<port>/Streaming/Channels/101
子码流：rtsp://<ip>:<port>/Streaming/Channels/102
```

开启鉴权后，项目还会生成带用户名和密码的展示 URL：

```text
rtsp://<user>:<password>@<ip>:<port>/Streaming/Channels/101
rtsp://<user>:<password>@<ip>:<port>/Streaming/Channels/102
```

需要注意，URL 中携带密码只是客户端使用形式；真正的认证是 RTSP Digest 认证，不是服务器直接信任 URL 字符串。

## 2. 代码分层与职责边界

### 2.1 IPC 业务层

关键文件：

- `Hi3516/hi3516_ipc/main_app/stream_main.cpp`
- `Hi3516/hi3516_ipc/main_app/stream_media/video/stream_video.cpp`
- `Hi3516/hi3516_ipc/main_app/stream_media/video/venc_channel_handler.cpp`
- `Hi3516/hi3516_ipc/main_app/stream_media/audio/stream_audio.cpp`
- `Hi3516/share/ipc_share/push_stream/push_stream.cpp`
- `Hi3516/share/ipc_share/push_stream/rtsp/rtsp_server.cpp`
- `Hi3516/share/ipc_share/push_stream/rtsp/rtsp_server.h`
- `Hi3516/share/ipc_share/push_stream/common/frame_queue.h`

这一层负责：

- 启停 RTSP 服务；
- 主码流和子码流路由；
- 根据配置选择视频、音频格式；
- 管理待发送帧队列；
- 响应客户端开始/停止状态；
- 请求海思 VENC 立即产生 IDR；
- 处理端口、鉴权、QoS、网络地址和编码配置变化。

> 本节的文件清单只是导航，不是代码讲解的终点。IPC 层的真实代码已在第二部分逐函数展开：初始化见第 34、39 章；VENC 取流和 NAL 解析见第 35、36 章；主子码流路由见第 37、38 章；RTSP session、队列和回调见第 42-49 章；动态配置见第 86-88 章。每章均引用实际代码并解释关键语句、线程、所有权和失败路径。

### 2.2 项目对 live555 的 C 接口封装层

关键文件：

- `Hi3516/share/cam_share/media/live555/mediaServer/rtspServer_base.h`
- `Hi3516/share/cam_share/media/live555/mediaServer/rtspServer_base.cpp`
- `Hi3516/share/cam_share/media/live555/mediaServer/custom_define.h`

这一层把 live555 的 C++ 对象模型包装成 IPC 业务层更容易调用的接口：

```cpp
rtsp_server_init(...);
rtsp_server_create(...);
rtsp_server_destory(...);
rtsp_server_unInit(...);
```

同时定义两个非常重要的回调：

- `FrameCallBack`：live555 向上层取一帧数据；
- `ClientStreamStatus`：live555 通知上层客户端开始、暂停或停止。

### 2.3 live555 媒体适配层

关键文件：

- `h264_server_subsession.cpp` / `h264_video_source.cpp`
- `h265_server_subsession.cpp` / `h265_video_source.cpp`
- `mjpeg_server_subsession.cpp` / `mjpeg_video_source.cpp`
- `aac_server_subsession.cpp` / `aac_audio_source.cpp`
- `G711aAudioStreamServerMediaSubsession.cpp`
- `G711uAudioStreamServerMediaSubsession.cpp`
- `G726AudioStreamServerMediaSubsession.cpp`

这一层负责把项目的“帧回调”适配为 live555 的两个核心抽象：

- `FramedSource`：媒体数据源，live555 从这里异步取帧；
- `OnDemandServerMediaSubsession`：一个 SDP track，例如视频 track 或音频 track。

### 2.4 live555 协议内核层

关键文件：

- `liveMedia/RTSPServer.cpp`
- `liveMedia/ServerMediaSession.cpp`
- `liveMedia/OnDemandServerMediaSubsession.cpp`
- `liveMedia/MultiFramedRTPSink.cpp`
- `liveMedia/H264or5VideoRTPSink.cpp`
- `liveMedia/H264VideoRTPSink.cpp`
- `liveMedia/H265VideoRTPSink.cpp`
- `liveMedia/JPEGVideoRTPSink.cpp`
- `liveMedia/MPEG4GenericRTPSink.cpp`
- `liveMedia/SimpleRTPSink.cpp`
- `liveMedia/RTCP.cpp`
- `liveMedia/RTPInterface.cpp`
- `liveMedia/DigestAuthentication.cpp`

这一层才真正实现 RTSP、SDP、RTP、RTCP、Digest 和 RTP-over-TCP。

## 3. 构建边界：源码目录不等于设备实际运行代码

IPC 主程序会编译：

```text
Hi3516/share/ipc_share/push_stream/rtsp/rtsp_server.cpp
```

但 live555 内核不是随主程序直接从 `share/cam_share/media/live555` 一起编译，而是通过以下 CMake 引入预编译库：

- `Hi3516/hi3516_ipc/ipc.cmake` 引入 `ipc_platform/lib/RtspServer/RtspServer.cmake`；
- `Hi3516/hi3516_ipc/main_app/CMakeLists.txt` 链接 `libRtspServer`；
- 实际库文件是 `Hi3516/ipc_platform/lib/RtspServer/lib/libRtspServer.so`。

这会直接影响排障结论：

- 修改 `rtsp_server.cpp` 后重编 IPC 主程序，修改会进入主程序；
- 只修改 `share/cam_share/media/live555` 下的源码，但不重新生成、替换并打包 `libRtspServer.so`，目标板运行行为不会改变；
- 分析源码时必须区分“当前源码镜像的行为”和“设备所带 `.so` 的真实行为”。

如果出现“源码明明改了，但设备仍然是旧行为”，第一项就应检查目标根文件系统中的 `.so` 哈希、时间和依赖加载路径。

## 4. 启动与关闭顺序

### 4.1 启动

`stream_main.cpp` 中的顺序是：

```text
CConfigManager::init
    -> CCryptoInit::init
    -> CStreamVideo::init
    -> CStreamAudio::init
    -> CPushStream::init
    -> ControlManage::init
```

RTSP 在 `CPushStream::init()` 中按配置启动：

```cpp
if (stInfo.bEnRtsp && !CRtspServer::instance()->isInit()) {
    CRtspServer::instance()->init();
}
```

视频先于 RTSP 初始化的原因是，RTSP 客户端一旦发起 DESCRIBE 或 PLAY，live555 可能立刻向上层取帧并请求 IDR；此时 VENC 必须已经存在。

`CStreamVideo::initCallbackBinding()` 还会注册 RTSP 到 VENC 的 IDR 回调：

```text
CRtspServer::triggerRequestIdr(channel)
    -> CStreamVideo::request_idr(channel)
    -> mppVenc_request_idr(...)
    -> ss_mpi_venc_request_idr(..., TD_TRUE)
```

### 4.2 关闭

关闭时顺序相反，且源码明确要求先停推流：

```text
ControlManage::deinit
    -> CPushStream::deinit
    -> CStreamAudio::deinit
    -> CStreamVideo::deinit
```

原因是如果先销毁 VENC/AENC，而 RTSP 的事件循环、媒体源或回调仍在取数据，就可能访问已经释放的音视频对象，或者在退出阶段等待永远不会再到达的数据。

## 5. RTSP 服务实例如何创建

`CRtspServer::init()` 做了四层初始化。

### 5.1 创建 IPC 侧总上下文

分配 `LIVE_RTSP_S`，保存：

- RTSP 端口；
- live555 服务器句柄；
- 主、子码流 `Live_Stream_Info_t`；
- 每路流的 URL、队列和请求状态。

### 5.2 创建 live555 全局环境

调用：

```cpp
rtsp_server_init(port, authentication, user, password,
                 digestAlgorithm, mediaDscp);
```

`rtsp_server_init()` 内部创建：

```text
BasicTaskScheduler
    -> BasicUsageEnvironment
    -> UserAuthenticationDatabase（可选）
    -> RTSPServer
    -> control EventTrigger
    -> event loop pthread
    -> error print pthread
```

`TaskScheduler` 是 live555 的事件调度中心。RTSP socket、RTP/RTCP socket、延迟取帧任务、跨线程控制事件都由它调度。

### 5.3 创建两路 ServerMediaSession

循环创建：

```text
Streaming/Channels/101
Streaming/Channels/102
```

每个 `ServerMediaSession` 表示一个可由 RTSP URL 访问的媒体会话。一个会话中包含：

- 一个视频 `ServerMediaSubsession`；
- 如果是复合流，再包含一个音频 `ServerMediaSubsession`。

也就是说，主码流与子码流是两个独立 RTSP session；每个 session 内的视频和音频是两个 track。

### 5.4 创建帧队列并注册回调

每路建立：

```cpp
videoQueue = std::make_unique<CThreadSafeFrameQueue>(MAX_VIDEO_FRAME);
audioQueue = std::make_unique<CThreadSafeFrameQueue>(MAX_AUDIO_FRAME);
```

普通设备默认：

- 视频队列 4 项；
- 音频队列 4 项。

特定 3882TI/3881T 宏配置下：

- 视频队列 32 项；
- 音频队列 16 项。

注册关系是：

```cpp
m_stClientInfo[i].clientFun = rtspStateCallback;
m_stClientInfo[i].dataGetfun = rtspFrameCall;
m_stClientInfo[i].Videoindex = listLive[i];
m_stClientInfo[i].Audioindex = composite ? listLive[i] : nullptr;
```

`Videoindex`、`Audioindex` 在这里本质上不是数字索引，而是透传给回调的 `Live_Stream_Info_t*`。

## 6. 视频数据完整链路

### 6.1 VENC 取流

`CStreamVideo` 的 VENC 线程从海思编码器取得 `ot_venc_stream`。一帧编码结果可能包含多个 `pack`，源码逐 pack 处理：

```text
mppVenc_get_stream
    -> stFrame.pack[i].addr + offset
    -> createFrame(channel, data, length)
    -> channelHandler->handleFrame(...)
    -> mppVenc_release_stream
```

这里必须理解两个层级：

- `ot_venc_stream` 是一次从 VENC 取出的编码结果；
- 项目传给 RTSP 的 `VideoFrame_S` 是其中一个 pack，而不一定等于一幅完整图像。

对于 H.264/H.265，pack 常表现为 Annex-B NAL 数据，开头通常有 `00 00 00 01`。后续 `H264VideoStreamFramer` / `H265VideoStreamFramer` 会继续按 NAL 和 Access Unit 解析。

### 6.2 NAL 类型识别

`createFrame()` 把 pack 复制进 `VideoFrame_S`，并用策略类识别 NAL 类型：

- H.264：读取 `pData[4] & 0x1F`；
- H.265：读取 `(pData[4] >> 1) & 0x3F`。

这隐含假设 pack 使用 4 字节 Annex-B 起始码。若编码器改成 3 字节起始码，或者一个 pack 前面不是起始码，这个解析会错位。

识别结果主要用于“起播边界”判断：

- H.264 SPS 被标记为 `iFrame=1`；
- H.265 VPS 被标记为 `iFrame=1`；
- MJPEG 每一帧都标记为 1。

源码字段叫 `iFrame`，但 H.264/H.265 中它实际表示“一个可解码随机访问序列的参数集起点”，并不是严格意义上的 IDR NAL。

### 6.3 主、子码流路由

`CMainChannelHandler::handleFrame()`：

```text
主码流 VideoFrame_S
    -> CPushStream::sendVideoData(frame, true, true)
    -> CRtspServer::sendVideoData(RTSP_CHN_MAIN, frame)
```

主码流为 SVAC3 时明确不送 RTSP。

`CSubChannelHandler::handleFrame()`：

```text
子码流 VideoFrame_S
    -> CPushStream::sendVideoData(frame, false, true)
    -> CRtspServer::sendVideoData(RTSP_CHN_SUB, frame)
```

子码流当前没有与主码流相同的 SVAC3 过滤判断。如果子码流被配置为 live555 不支持的编码，RTSP 创建阶段和实际数据类型可能不一致，这是配置边界风险。

### 6.4 只在有客户端请求时入队

`CRtspServer::sendVideoData()` 检查：

```cpp
if (pStreamInfo->request == 1) {
    // 复制并入队
} else {
    videoQueue->clear();
}
```

这是按需发送设计：没有 RTSP 客户端时不让队列持续堆积，也避免为 RTSP 额外分配内存。

每个 pack 入队时发生一次深拷贝：

```text
VideoFrame_S::pData -> FrameData::data
```

队列满时 `push()` 返回 `false`，当前新帧被丢弃，旧帧继续保留。

### 6.5 live555 反向拉取

live555 的视频源不是主动等待业务线程 push，而是调度 `doGetNextFrame()`：

```text
H264/H265 Video Source::doGetNextFrame
    -> scheduleDelayedTask
    -> getNextFrame1
    -> dataGetfun(&Fream_Info_t)
    -> rtspFrameCall
    -> videoQueue.pop
    -> memcpy 到 live555 的 fTo
    -> afterGetting
    -> RTP Sink 封包并发送
```

队列空时，H.264/H.265 数据源延迟 10 ms 再试。虽然队列类提供 `condition_variable` 和 `pop_wait()`，当前 RTSP 取帧路径没有使用阻塞等待，而是 live555 定时轮询。

## 7. 新客户端为什么必须请求 IDR

H.264/H.265 的 P/B 帧依赖之前的参考帧。客户端在码流中途加入时，如果第一批收到的是 P 帧，即使网络和 RTP 都正常，也无法立即解码。

项目在客户端开始时执行：

```text
RTSP PLAY 或媒体源首次取帧
    -> RTSPCLIENT_START
    -> request = 1
    -> requestIFrame = 1
    -> triggerRequestIdr(channel)
    -> ss_mpi_venc_request_idr(channel, TD_TRUE)
```

之后 `rtspFrameCall()` 在 `requestIFrame==1` 时不断丢弃普通 NAL，直到：

- H.264 遇到 SPS；
- H.265 遇到 VPS；
- MJPEG 遇到任意图像。

这样期望队列后续顺序是：

```text
H.264：SPS -> PPS -> IDR slices -> 后续 P/B
H.265：VPS -> SPS -> PPS -> IDR/CRA slices -> 后续 P/B
```

为什么不直接把 IDR NAL 标为起点？因为解码器除了 IDR 图像还需要参数集；从 SPS/VPS 开始保留整段随机访问序列更合理。

`triggerRequestIdr()` 对同一通道设置 30 ms 最小间隔，防止 DESCRIBE、PLAY、多个客户端或多个状态回调瞬间重复轰炸 VENC。

## 8. RTSP 控制协议章

RTSP 是控制面协议，它不直接承载绝大多数媒体数据。媒体通常通过 RTP 传输，RTCP负责统计与同步。

### 8.1 典型请求顺序

```text
客户端                         IPC/live555
   |---- OPTIONS ----------------->|
   |<--- 200 + Public -------------|
   |---- DESCRIBE ---------------->|
   |<--- 401 Digest challenge -----|  开启鉴权时
   |---- DESCRIBE + Authorization >|
   |<--- 200 + SDP ----------------|
   |---- SETUP video track -------->|
   |<--- Transport + Session -------|
   |---- SETUP audio track -------->|
   |<--- Transport + Session -------|
   |---- PLAY --------------------->|
   |<--- Range + RTP-Info ----------|
   |<=== RTP video/audio ===========|
   |<==> RTCP SR/RR ===============>|
   |---- GET_PARAMETER ------------>|
   |<--- 200 keepalive -------------|
   |---- TEARDOWN ----------------->|
   |<--- 200 -----------------------|
```

### 8.2 OPTIONS

`RTSPServer::RTSPClientConnection::handleCmd_OPTIONS()` 返回：

- RTSP 版本；
- CSeq；
- Date；
- `Public` 支持的方法列表。

它主要用于客户端探测服务能力。

### 8.3 DESCRIBE

处理路径：

```text
handleCmd_DESCRIBE
    -> authenticationOK
    -> lookupServerMediaSession(url suffix)
    -> ServerMediaSession::generateSDPDescription
    -> Content-Type: application/sdp
```

对 `/Streaming/Channels/101`，live555 查找同名 `ServerMediaSession`，再拼接会话级 SDP 和各 track 的 SDP。

一个容易忽略的行为是：为获得 H.264 SPS/PPS 或 H.265 VPS/SPS/PPS，DESCRIBE 阶段可能已经启动一个 dummy RTP sink 并开始向上层取数据。因此“客户端还没 PLAY，VENC 就收到 IDR 请求或 RTSP 队列开始工作”可能是正常的 SDP 探测行为。

### 8.4 SETUP

SETUP 针对 SDP 中的具体 track，例如：

```text
.../Streaming/Channels/101/track1
.../Streaming/Channels/101/track2
```

处理逻辑：

1. 找到 session 和 track；
2. 解析 `Transport` 头；
3. 判断 RTP/UDP、RTP/TCP 或 raw UDP；
4. 为 UDP 分配服务器 RTP/RTCP 端口；
5. 为 TCP 记录 RTSP socket 和 interleaved channel id；
6. 调用 `OnDemandServerMediaSubsession::getStreamParameters()`；
7. 返回 `Session` 和协商后的 `Transport`。

UDP 响应形式类似：

```text
Transport: RTP/AVP;unicast;
 client_port=50000-50001;
 server_port=6970-6971
```

TCP 形式类似：

```text
Transport: RTP/AVP/TCP;unicast;interleaved=0-1
```

### 8.5 PLAY

PLAY 处理：

1. 解析可选的 `Scale`、`Range`；
2. 对实时源通常返回 `Range: npt=...-`；
3. 对每个目标 track 调用 `startStream()`；
4. 返回当前 RTP sequence 和 timestamp 的 `RTP-Info`；
5. 项目的视频 subsession 再向 IPC 层发出 `RTSPCLIENT_START`。

真正启动 RTP sink 的位置在：

```text
OnDemandServerMediaSubsession::startStream
    -> StreamState::startPlaying
    -> RTCPInstance::createNew
    -> RTPSink::startPlaying(mediaSource)
```

### 8.6 PAUSE

项目的视频 subsession 使用 `reuseFirstSource=True`。live555 的默认实现明确规定：共享同一 source 时不允许单个客户端暂停公共 source，因此 `pauseStream()` 直接返回。

结果是客户端可能收到 PAUSE 200，但公共媒体源不会因一个客户端暂停而停下来。这是共享源多客户端模型的自然结果。

### 8.7 GET_PARAMETER / SET_PARAMETER

当前实现把二者作为 no-op keepalive：

- `GET_PARAMETER` 返回 200 和一个简单内容；
- `SET_PARAMETER` 返回空的 200。

它们没有接入设备参数读写。IPC 配置走项目自己的控制面、HTTP/HTTPS、TVSDK 等链路，而不是 RTSP SET_PARAMETER。

### 8.8 TEARDOWN

TEARDOWN 会：

```text
remove RTP destination
    -> remove RTCP destination/RR handler
    -> decrement StreamState reference
    -> last reference 时销毁 source、sink、groupsock、RTCP
```

项目重写的 `deleteStream()` 尝试在“最后一个客户端”离开时通知 IPC：

```text
RTSPCLIENT_STOP
    -> request = 0
    -> requestIFrame = 0
    -> 后续业务帧不再入队，已有队列被清空
```

## 9. SDP 协议章

SDP 是媒体说明，不传媒体本身。客户端必须先通过 SDP 知道：

- 有哪些 track；
- 每个 track 是 video 还是 audio；
- 使用什么 codec；
- RTP payload type；
- RTP clock rate；
- H.264/H.265 参数集；
- AAC AudioSpecificConfig；
- track control URL。

### 9.1 会话级 SDP

`ServerMediaSession::generateSDPDescription()` 生成：

```text
v=0
o=- <session-id> <version> IN IP4 <device-ip>
s=<description>
i=<info>
t=0 0
a=tool:LIVE555 Streaming Media...
a=type:broadcast
a=control:*
a=range:npt=now-
```

实时流 duration 为 0，因此使用 `a=range:npt=now-`，表示没有固定文件时长。

### 9.2 track 级 SDP

`OnDemandServerMediaSubsession::setSDPLinesFromRTPSink()` 生成：

```text
m=<media> <port> RTP/AVP <payload-type>
c=IN IP4 <address>
b=AS:<estimated-bitrate>
a=rtpmap:...
a=fmtp:...
a=control:trackN
```

动态 payload type 按 `96 + trackNumber - 1` 分配。通常视频是 96，音频是 97；但 G.711 8 kHz 单声道会改用静态 payload 0 或 8。

### 9.3 为什么 H.264/H.265 DESCRIBE 会取帧

H.264 SDP 需要：

```text
profile-level-id
sprop-parameter-sets=<base64 SPS>,<base64 PPS>
```

H.265 SDP 需要：

```text
sprop-vps=<base64 VPS>
sprop-sps=<base64 SPS>
sprop-pps=<base64 PPS>
```

这些参数不是 RTSP 服务启动时从视频配置结构直接构造，而是 RTP sink 从真实码流中解析得到。因此 `getAuxSDPLine()`：

1. 创建 dummy source 和 dummy RTP sink；
2. 启动读取；
3. 每 100 ms 检查参数集是否齐全；
4. 进入一个带 `fDoneFlag` 的事件循环；
5. 参数集生成后缓存 SDP fmtp。

如果 VENC 没有输出参数集、RTSP 上层没有置 `request=1`、队列满后参数集被丢弃，DESCRIBE 可能长时间卡住或 SDP 缺少 fmtp。

## 10. UDP、TCP 与 interleaved 传输章

### 10.1 RTP over UDP

这是传统模式：

- RTSP 控制连接使用 TCP；
- 视频 RTP 使用一个 UDP 端口；
- 视频 RTCP 使用相邻 UDP 端口；
- 音频再使用另一对 UDP 端口。

live555 为 RTP 选择偶数端口，为 RTCP 选择后续奇数端口，并把客户端地址和 `client_port` 加到 `Groupsock` destination。

优点：

- 无 TCP 队头阻塞；
- 实时性好；
- RTP/RTCP 语义天然清晰。

缺点：

- 容易被防火墙、NAT 或企业网络拦截；
- 丢包不会自动重传；
- 需要开放动态 UDP 端口。

### 10.2 RTP over RTSP/TCP

客户端请求：

```text
Transport: RTP/AVP/TCP;unicast;interleaved=0-1
```

此时 RTP 和 RTCP 与 RTSP 共用同一 TCP socket，每个数据块前有 `$`、channel id 和长度：

```text
$ <channel> <16-bit length> <RTP/RTCP packet>
```

项目中 `StreamState::startPlaying()` 调用：

```text
RTPSink::addStreamSocket(tcpSocket, rtpChannel)
RTCPInstance::addStreamSocket(tcpSocket, rtcpChannel)
```

并注册 alternative byte handler，保证同一 socket 一边发送 RTP，一边仍能解析客户端后续的 RTSP 请求。

优点：

- 更容易穿过防火墙；
- ffplay 使用 `-rtsp_transport tcp` 时更稳定；
- 网络丢包由 TCP 重传。

缺点：

- 丢包会引起队头阻塞；
- 网络抖动时延迟可能不断增长；
- RTSP 控制和媒体共享连接，一个慢客户端对 socket 写入更敏感。

### 10.3 当前未启用的能力

live555 源码具备 multicast、RTSP-over-HTTP、SRTP/TLS 等通用分支，但项目当前创建的是按需 unicast session，初始化时没有开启 SRTP/TLS，也没有配置 HTTP tunnel 端口。排障时不要因为源码中存在分支就认为产品已经启用。

## 11. RTP 通用协议章

RTP 包头的核心字段：

- Payload Type：告诉客户端负载格式；
- Sequence Number：每个 RTP 包递增，用于检测丢包和排序；
- Timestamp：媒体时钟时间，不是 Unix 时间；
- SSRC：标识同步源；
- Marker：由具体 codec 定义帧边界。

`MultiFramedRTPSink` 默认：

- 首选 payload 约 1000 字节；
- 最大 RTP payload 1452 字节；
- 加上 RTP、UDP、IP 头后尽量适应常见 1500 MTU。

项目把 `OutPacketBuffer::maxSize` 调到 2.5 MiB。这个值是“读取一个大 codec frame/NAL 时的内部缓冲上限”，不是说一个 UDP 包会达到 2.5 MiB。H.264/H.265 的大 NAL 仍会被分成多个约 MTU 大小的 FU 包。

### 11.1 从队列到网络的第二次拷贝

`rtspFrameCall()` 把 `FrameData::data` 复制到 live555 的 `FramedSource::fTo`。之后 RTP sink 会把负载组织到 `OutPacketBuffer`。

所以 RTSP 上层至少存在：

```text
VENC pack -> VideoFrame_S              第一次业务层复制
VideoFrame_S -> FrameData 队列          第二次业务层复制
FrameData -> live555 fTo                第三次业务层复制
fTo/NAL -> RTP OutPacketBuffer          live555 封包复制/移动
```

不能把本链路概括为“只有一次拷贝”。海思 MPP 从 VENC 取流本身可以避免复制编码器内部缓冲，但业务层为了跨线程和跨生命周期安全，做了多次深拷贝。

## 12. RTCP 协议章

RTCP 与 RTP 配套，主要负责：

- Sender Report，SR；
- Receiver Report，RR；
- Source Description，SDES/CNAME；
- BYE；
- 发送和接收统计。

### 12.1 服务端发送 SR

`StreamState::startPlaying()` 创建 `RTCPInstance` 后立即 `sendReport()`，使客户端尽早拿到 RTP timestamp 与 NTP wall-clock 的映射。

SR 中包含：

- NTP 时间；
- 当前 RTP timestamp；
- 已发送 RTP 包数；
- 已发送字节数。

客户端可用它把不同 clock rate 的音频和视频映射到共同时间轴。

### 12.2 客户端发送 RR

RR 包含：

- 丢包比例和累计丢包；
- 最高 sequence；
- interarrival jitter；
- 上次 SR 时间；
- 从上次 SR 到当前的延迟。

live555 会解析 RR 并更新 transmission statistics，同时通过 RR handler 记录客户端活跃状态。

当前 IPC 业务层没有把这些统计暴露为可观测指标，因此出现卡顿时通常只能靠抓包或扩展日志查看 RR。

### 12.3 BYE

`RTCPInstance` 销毁时会发送 BYE。不过共享源模式下，单个客户端 TEARDOWN 时源码刻意禁用了向所有客户端发送 BYE 的逻辑，否则一个客户端离开会误伤仍在播放的客户端。

## 13. H.264 over RTP 协议章

### 13.1 输入格式

项目从 VENC 得到 Annex-B 字节流 pack，通常包括起始码和 NAL：

```text
00 00 00 01 67 ...   SPS
00 00 00 01 68 ...   PPS
00 00 00 01 65 ...   IDR
00 00 00 01 41 ...   non-IDR slice
```

`H264VideoStreamFramer` 解析起始码、NAL 和 Access Unit；RTP payload 中不会携带 Annex-B 起始码。

### 13.2 SDP

`H264VideoRTPSink::auxSDPLine()` 从 SPS/PPS 生成：

```text
a=rtpmap:96 H264/90000
a=fmtp:96 packetization-mode=1;
 profile-level-id=...;
 sprop-parameter-sets=<SPS>,<PPS>
```

90 kHz 是视频 RTP 时钟。它与实际 25/30 fps 不是同一个概念：30 fps 时相邻图像 timestamp 通常增加 3000。

### 13.3 单 NAL 与 FU-A 分片

NAL 小于单包容量时直接作为 RTP payload。

NAL 太大时，`H264or5Fragmenter` 使用 H.264 FU-A：

```text
原 NAL header
    -> FU indicator(type=28)
    -> FU header(S/E/type)
    -> 分片 payload
```

- 第一片设置 S；
- 中间片 S/E 都不设置；
- 最后一片设置 E；
- 同一个 Access Unit 的所有 NAL/FU 分片使用同一个 RTP timestamp；
- Access Unit 最后一个 NAL 的最后一个 RTP 包设置 Marker。

### 13.4 起播关键点

客户端至少需要 SPS、PPS 和 IDR。若只收到 IDR 而 SDP 没有参数集，某些客户端仍无法解码；若只收到 SPS/PPS 而后续 IDR 在队列满时被丢弃，也会黑屏。

## 14. H.265 over RTP 协议章

### 14.1 输入格式

典型序列：

```text
00 00 00 01 40 ...   VPS
00 00 00 01 42 ...   SPS
00 00 00 01 44 ...   PPS
00 00 00 01 26 ...   IDR/IRAP 类数据
```

H.265 NAL header 为 2 字节，项目的 NAL 类型解析从起始码后的第一个 header 字节取 bit 1..6。

### 14.2 SDP

`H265VideoRTPSink` 生成：

```text
a=rtpmap:96 H265/90000
a=fmtp:96 sprop-vps=...;sprop-sps=...;sprop-pps=...
```

缺少任一参数集都可能导致客户端协商成功但无法解码。

### 14.3 FU 分片

H.265 大 NAL 使用 type 49 的 Fragmentation Unit：

- 两字节 Payload Header；
- 一字节 FU Header；
- S/E 标记分片首尾；
- 同一 Access Unit 共享 RTP timestamp；
- 最后一片按 picture end 设置 Marker。

### 14.4 当前时间戳实现的关注点

H.264 自定义 source 在 presentation time 为空或未变化时调用 `gettimeofday()`。H.265 自定义 source 当前没有同样直接设置 `fPresentationTime` 的代码，而更多依赖 H.265 framer 的行为。

如果出现 H.265 可播放但速度异常、RTP timestamp 不增长、音画同步漂移，应优先抓包比较 H.264/H.265 timestamp 增量，并核对实际 `.so` 是否包含对应修复。

## 15. MJPEG over RTP 协议章

MJPEG 是连续 JPEG 图像。项目链路是：

```text
VENC MJPEG pack
    -> MJPEG_FRAME_SOURCE
    -> MJPEG_Video_Source 解析 JPEG marker、量化表、宽高
    -> JPEGVideoRTPSink
    -> RFC 2435 JPEG RTP payload
```

与 H.264/H.265 不同：

- 每张图像独立，不依赖参考帧；
- 不需要请求 IDR；
- 每帧都可以作为随机访问点；
- 带宽通常显著更高；
- RTP sink 需要从 JPEG 头提取 type、Q、宽高、restart interval、量化表等信息。

项目为 MJPEG SDP 额外写入固定的 `b=AS:4096`、`a=framerate:30` 和实际宽高。这里的 4096/30 是硬编码，不一定与运行配置一致。

## 16. AAC over RTP 协议章

### 16.1 AENC 输出与 ADTS 去除

海思 AAC 编码器配置为 ADTS transport。`CStreamAudio` 取到 AAC 帧后判断：

- `protection_absent=1`：ADTS 头 7 字节；
- 有 CRC：ADTS 头 9 字节。

然后为 RTSP 创建的 `AudioFrame_S` 只保存 ADTS 后面的 AAC raw_data_block。

这是正确的，因为 RTP MPEG4-GENERIC 不直接承载 ADTS 头；采样率、声道和 AAC object type 通过 SDP `config` 提供。

### 16.2 AudioSpecificConfig

`aacAudioSource` 根据：

- AAC LC object type；
- samplingFrequencyIndex；
- channelConfiguration；

构造 2 字节 AudioSpecificConfig，再转为十六进制字符串。

项目映射：

- 8 kHz -> index 11；
- 16 kHz -> index 8；
- 32 kHz -> index 5；
- 48 kHz -> index 3；
- 默认单声道。

### 16.3 SDP 与 RTP

`MPEG4GenericRTPSink` 产生类似：

```text
a=rtpmap:97 MPEG4-GENERIC/16000/1
a=fmtp:97 streamtype=5;profile-level-id=1;
 mode=AAC-hbr;config=<由当前 AudioSpecificConfig 代码生成>;
 SizeLength=13;IndexLength=3;IndexDeltaLength=3
```

RTP payload 中带 AU Header Section，用 AU-size 指明后续 AAC Access Unit 大小。需要特别注意：当前源码把 `profile=1` 直接写入 AudioSpecificConfig 的 audioObjectType 字段，按 MPEG-4 定义会得到 object type 1（AAC Main），而常见 AAC-LC 应是 object type 2；具体 `config` 必须以当前代码生成值和 AENC 实际 profile 抓包核对。

### 16.4 AAC 时间戳

AAC-LC 每帧通常 1024 samples，因此：

```text
duration_us = 1024 * 1,000,000 / sample_rate
```

例如 16 kHz 时约 64 ms。AAC source 使用这个持续时间递增 presentation time，并在队列积压时把 duration 置 0 加速清空。

## 17. G.711 over RTP 协议章

### 17.1 G.711 μ-law

8 kHz、单声道时使用静态 payload type 0：

```text
PT=0, PCMU/8000/1
```

### 17.2 G.711 A-law

8 kHz、单声道时使用静态 payload type 8：

```text
PT=8, PCMA/8000/1
```

其他采样率或声道组合使用动态 payload type，并通过 `a=rtpmap` 描述。

`SimpleRTPSink` 被设置为不把多帧聚合进一个 RTP 包，使上层的每个音频帧边界更直接地映射到 RTP 发送。

G.711 的 timestamp clock 等于采样率。8 kHz 时每 20 ms、160 samples 的帧使 RTP timestamp 增加 160。

## 18. G.726 over RTP 协议章

项目当前主要支持 G.726-32：

- 8 kHz；
- 4 bit/sample；
- 32 kbit/s；
- 动态 payload type。

源码也映射：

- 2 bit -> `G726-16`；
- 3 bit -> `G726-24`；
- 4 bit -> `G726-32`；
- 5 bit -> `G726-40`。

G.726 source 根据：

```text
samples = frameBytes * 8 / bitsPerSample
duration = samples / sampleRate
```

计算每帧持续时间。

G.726 在不同系统中还可能存在 AAL2 packing 与 RFC packing 的位序差异。当前源码的 SDP MIME 只能描述码率，不能自动消除位序不兼容。如果 NVR 能协商却只有噪声，要核对编码器输出 packing 和客户端期望。

## 19. Digest 鉴权协议章

### 19.1 服务启动

`rtsp_server_init()` 在启用鉴权时创建：

```cpp
UserAuthenticationDatabase("Itc Streaming Server", False);
authDB->addUserRecord(user, password);
```

用户名和明文密码保存在内存认证库中。

### 19.2 挑战响应

首次 DESCRIBE/SETUP 没有合法 Authorization 时：

```text
RTSP/1.0 401 Unauthorized
WWW-Authenticate: Digest realm="...", nonce="...", algorithm=...
```

客户端计算摘要后重试。服务器校验：

```text
HA1 = HASH(username:realm:password)
HA2 = HASH(method:uri)
response = HASH(HA1:nonce:HA2)
```

项目扩展支持：

- MD5；
- SHA-256；
- 同时下发 MD5 和 SHA-256 challenge 的组合模式。

鉴权主要在 DESCRIBE 和 SETUP 阶段完成，后续 PLAY 依赖已建立的 RTSP session。

### 19.3 安全注意事项

- RTSP 本身不是 TLS，Digest 保护的是密码摘要，不加密 URL、SDP 或媒体；
- RTP 也未启用 SRTP，视频内容可被同网段抓包；
- `updateNetworkConfig()` 当前会把带账号密码的 URL 写日志，日志权限必须受控；
- URL 生成没有做用户名/密码 percent-encoding，特殊字符可能破坏 URI；
- MD5 兼容性最好但安全性弱，SHA-256 需要确认所有 NVR/客户端兼容。

## 20. 多客户端与共享源模型

H.264、H.265、MJPEG 和音频 subsession 都使用 `reuseFirstSource=True`。含义是：

- 第一个客户端 SETUP 时创建 `StreamState`、source、sink、RTP/RTCP socket；
- 后续客户端复用同一个 source 和 RTP sink；
- 每个客户端只增加一个 destination；
- 同一编码数据由一个 RTP sink 分发给多个目标。

优点：

- 不会为每个客户端重复从业务队列取一份数据；
- CPU 和内存开销低；
- 所有客户端共享同一 sequence/timestamp 流。

代价：

- 一个客户端不能单独暂停公共 source；
- 无法为不同客户端独立快进、seek 或调速；
- 客户端起播边界、公共队列和 IDR 请求相互影响；
- TCP 慢客户端的发送状态需要重点观察。

### 20.1 当前连接数限制的实际情况

IPC 层把 `param1` 设为 4，并在 `rtsp_server_create()` 中调用 `rtsp_setclient_maxNum()`。

但调用发生在 `ServerMediaSession::createNew()` 之前，按当前源码 `findstreamName()` 会查找失败，返回值又被忽略。因此这个“每路 4 客户端”设置很可能没有真正写入 session。

此外：

- `ServerMediaSession::fReferenceMax` 在当前代码中没有看到实际拒绝连接的判断；
- `RTSPServer.cpp` 另有全局 TCP 连接上限 32；
- `CLIENTMAX=4` 更多限制 `fClientInfo` 记录数组，而不是完整的连接准入机制。

所以“产品宣称最多 4 客户端”和“代码真正拒绝第 5 个客户端”不是同一件事，必须在目标板实测。

## 21. 线程模型

### 21.1 主要线程

```text
VENC 取流线程
    -> 创建 VideoFrame_S
    -> RTSP videoQueue.push

AENC/音频处理线程
    -> 创建 AudioFrame_S
    -> 主、子 audioQueue.push

live555 event loop 线程
    -> 解析 RTSP
    -> 调度 FramedSource
    -> videoQueue/audioQueue.pop
    -> RTP/RTCP socket 发送

RTSP error print 线程
    -> 每秒读取 UsageEnvironment 错误信息

控制业务线程
    -> init/deinit/reboot/修改配置
```

队列用 mutex 保护，帧内存用 `unique_ptr` 管理，这是生产者与消费者之间的主要线程安全边界。

### 21.2 live555 单线程规则

live555 大量对象默认要求在 event loop 线程访问。项目销毁 `ServerMediaSession` 时没有直接从控制线程删除，而是：

```text
控制线程
    -> scheduler->triggerEvent
    -> event loop 执行 cleanServerMediaSession
    -> condition_variable 通知控制线程
```

这是正确的线程归属处理方式。

### 21.3 当前数据竞争关注点

`Live_Stream_Info_t::request` 和 `requestIFrame` 是 `volatile int`。`volatile` 只限制编译器优化，不提供 C++ 线程同步语义。

它们由 live555 线程写、VENC/AENC 线程读，严格按 C++ 内存模型属于数据竞争。应使用 `std::atomic<int>` 或在统一 mutex 下访问。

## 22. 队列、背压与延迟

### 22.1 队列满策略

当前策略是：

```text
队列满 -> 拒绝新帧 -> 保留旧帧
```

这个策略对实时视频未必理想。保留旧帧意味着客户端继续播放过时内容，延迟不会立刻下降；同时新的 SPS/VPS/IDR 可能被丢弃。

实时预览更常见的策略是：

- 音频丢旧保新，限制累计时延；
- 视频在积压时清到最近的随机访问点；
- 参数集/IDR 到达时优先保留；
- 记录 drop-old、drop-new、queue high-watermark。

### 22.2 source 的加速清队列

H.264/H.265/AAC/G.711/G.726 source 看到队列深度较大时，会把 `fDurationInMicroseconds` 设为 0，要求 RTP sink 尽快继续取下一帧。

这能清理短时积压，但不能替代完善的背压策略：网络持续不足时，生产速度仍大于消费速度。

### 22.3 低帧率特殊处理

H.264/H.265 source 对低于 1 fps 的视频把 duration 置 0。原因是一个 Access Unit 可能拆成 SPS/PPS/SEI/IDR 多个 NAL；如果每个 NAL 都继承 2 秒 duration，sink 会错误累计成 8 秒。

这个修复说明项目的上层“队列项”与 live555 下层“完整图像”并不总是一一对应，调试 timestamp 时必须按 Access Unit 而不是按队列项计数。

## 23. 时间戳与音画同步

### 23.1 当前没有把 VENC PTS 传到 RTSP

VENC pack 中有 `pts`，诊断日志也能打印，但 `VideoFrame_S`、`FrameData`、`Fream_Info_t` 当前没有把该 PTS 完整传给 live555。

视频 presentation time 主要由：

- `gettimeofday()`；
- 配置 fps 推导的 duration；
- H.264/H.265 framer 的 Access Unit 逻辑；

重建。

音频也使用本地 wall clock 加每帧采样持续时间递增。

### 23.2 同步原理

RTP 音频和视频各自有独立 clock：

- 视频一般 90 kHz；
- AAC/G.711/G.726 等于其采样率。

RTCP SR 把两个 RTP clock 映射到 NTP wall clock，客户端据此同步。

### 23.3 可能的漂移来源

- VENC 真正采集节奏与配置 fps 不一致；
- 丢帧后 duration 仍按固定 fps 递增；
- 音频重采样或编码帧长度变化；
- 系统时间被 NTP/手动校时跳变；
- 队列积压时 duration 被置 0；
- H.265 presentation time 初始化不完整；
- 同一个编码 Access Unit 被拆成多个 pack/NAL。

长期方案应优先把硬件 PTS 沿数据结构传到 FramedSource，并建立统一单调时钟映射，避免直接依赖可跳变的 `gettimeofday()`。

## 24. 内存与资源生命周期

### 24.1 帧所有权

```text
MPP VENC pack
    所有者：海思 VENC，release_stream 后失效

VideoFrame_S
    所有者：CStreamVideo 当前循环
    必须在 release_stream 前完成独立复制

FrameData
    所有者：RTSP queue，unique_ptr

live555 fTo
    所有者：FramedSource/RTP sink 调用链
```

RTSP 队列必须深拷贝，原因是 `mppVenc_release_stream()` 后原 pack 地址会归还编码器，不能跨线程保留裸指针。

### 24.2 当前分配/释放不匹配

`CStreamVideo::createFrame()` 使用：

```cpp
malloc(sizeof(VideoFrame_S) + nDataLen)
```

但 `freeFrame()` 使用：

```cpp
delete pVideoFrame;
```

错误路径也使用 `delete`。这是 `malloc/delete` 不匹配，属于未定义行为，可能表现为偶发 heap corruption、长时间运行崩溃或退出崩溃。

它位于 RTSP/GB28181/录像共用的视频分发路径，排障优先级很高。正确方向是统一 `malloc/free`，或改为 RAII 容器/智能指针并保持分配器配对。

音频路径存在同样问题：`CStreamAudio::createFrame()` 用 `malloc` 分配 `AudioFrame_S`，`CStreamAudio::freeFrame()` 却使用 `delete`。因此这不是只影响视频的单点问题，而是音视频公共帧生命周期都需要统一修正。

### 24.3 含 C++ 对象的结构体被 calloc/free 管理

`Live_Stream_Info_t` 内含：

```cpp
std::unique_ptr<CThreadSafeFrameQueue> videoQueue;
std::unique_ptr<CThreadSafeFrameQueue> audioQueue;
```

但 `CRtspServer::init()` 使用 `calloc()` 分配整个 `Live_Stream_Info_t`，`deinit()` 使用 `free()` 释放。

这有两层问题：

1. `calloc` 不会调用 `std::unique_ptr` 构造函数，在未构造的 C++ 对象上执行赋值属于未定义行为；
2. `free` 不会调用 `std::unique_ptr` 析构函数，因此队列对象不能依赖 RAII 自动销毁。

当前代码虽然在 free 前调用了 queue `clear()`，但 `clear()` 只释放队列中的帧，不会销毁 `CThreadSafeFrameQueue` 对象本身，也不能修复“unique_ptr 从未构造”的问题。

这个结构应使用 `new/delete`、`std::make_unique<Live_Stream_Info_t>()`，或者把非平凡 C++ 成员移到正常构造的 class 中。

### 24.4 live555 最大帧缓冲

`REV_BUF_SIZE` 和 `OutPacketBuffer::maxSize` 是 2.5 MiB。`rtspFrameCall()` 直接按 `FrameData::frameSize` memcpy 到 `frame->data`，接口没有显式传入目标容量。

如果单个 VENC pack 超过 source 的实际缓冲容量，存在越界风险。应在回调接口加入 capacity，或至少把长度与统一上限比较并记录截断。

## 25. 配置变化与 RTSP reboot

以下变化可能触发 RTSP 重新初始化：

- RTSP 端口；
- 媒体 DSCP；
- 视频 codec、帧率、复合流/纯视频类型等；
- 音频格式或采样参数；
- Digest 算法；
- 用户认证相关配置。

`CRtspServer::reboot()`：

```text
lock m_mutexCtrl
    -> if initialized: deinit
    -> init
```

重启会断开现有客户端，删除两个 `ServerMediaSession`，清队列，再按新 codec 重新生成 SDP。

### 25.1 启停配置

`ConfigHttpsInfo` 中同时管理 `bEnRtsp`：

- true 且未初始化 -> `init()`；
- false 且已初始化 -> `deinit()`。

命名上它属于 HTTPS 配置，但实际也承载 RTSP enable，这一点接手时容易漏掉。

### 25.2 需要注意的行为

`setPort()` 和 `setQosDscp()` 无条件调用 `reboot()`。而 `reboot()` 即使原来未初始化，也会执行 `init()`。因此在 RTSP 被关闭时修改端口/QoS，存在意外重新开启服务的可能。

## 26. 已确认的高风险点

下面按排障和修复优先级排列。

### P0：可能导致崩溃或内存破坏

1. `VideoFrame_S` 和 `AudioFrame_S` 使用 `malloc` 分配、`delete` 释放。
2. 含 `std::unique_ptr` 的 `Live_Stream_Info_t` 使用 `calloc/free`，构造析构语义被绕过。
3. `rtspFrameCall()` memcpy 没有目标容量参数。
4. `rtsp_server_unInit()` 设置退出标志后没有 join event loop 线程，就释放 environment 和 scheduler，存在 event loop 继续访问已释放对象的风险。
5. `CRtspServer::sendVideoData/sendAudioData` 只用 init atomic 做门控，控制线程 deinit/reboot 与生产线程并发时仍可能在检查后访问已释放的 `m_pLiveInfo`。

### P1：黑屏、卡住或多客户端状态错误

1. 队列满时丢新帧，可能丢掉新的 SPS/VPS/IDR。
2. `request/requestIFrame` 使用 volatile 而不是 atomic。
3. START/STOP 可能来自 subsession、source 构造出的 dummy SDP 流和 source 析构，多处写同一个布尔 `request`，不是严格的引用计数状态机。
4. “I 帧”标记实际是 SPS/VPS；参数集与 IDR 中间若被打断，仍可能起播失败。
5. H.264/H.265 `getAuxSDPLine()` 需要真实参数集，编码器或队列异常会让 DESCRIBE 卡住。
6. H.265 presentation time 路径与 H.264 不对称。
7. 子码流缺少 SVAC3/不支持 codec 的显式过滤。

### P2：配置、容量与可观测性问题

1. `rtsp_server_create()` 在 session 创建前设置最大客户端数，返回值未检查。
2. `CPushStream::init()` 没有传播 `CRtspServer::init()` 返回值。
3. `CRtspServer::init()` 没有检查两次 `rtsp_server_create()` 返回值。
4. `cleanAllServerMediaSession()` 当前循环清理到第一项就 return。
5. H.264/MJPEG `estBitrate=2000000` 的单位在 live555 中是 kbps，可能导致异常 SDP `b=AS` 和过大的 socket buffer 请求；H.265 是 20000 kbps。
6. RTCP RR 丢包、jitter、RTT 未暴露到业务监控。
7. URL 日志可能泄露账号密码。
8. source 使用 10 ms 轮询，没有利用队列条件变量。

## 27. 常见故障的源码排查路径

### 27.1 端口无法监听

检查：

1. `bEnRtsp` 是否为 true；
2. `CPushStream::init()` 是否调用 `CRtspServer::init()`；
3. `rtsp_server_init()` 是否返回 null；
4. 554 是否被占用，非 root 运行时是否有低端口权限；
5. 目标加载的是否是预期 `libRtspServer.so`；
6. 防火墙是否允许 TCP 554。

### 27.2 能连 TCP，但 DESCRIBE 超时

重点检查：

1. Digest 401/Authorization 是否循环失败；
2. URL 大小写和 suffix 是否严格为 `Streaming/Channels/101`；
3. `getAuxSDPLine()` 是否等待 SPS/PPS 或 VPS/SPS/PPS；
4. DESCRIBE dummy source 是否已经触发 `request=1`；
5. VENC 是否输出参数集；
6. `rtspFrameCall()` 是否能从队列读到数据；
7. live555 event loop 是否卡在别的回调。

### 27.3 RTSP 200 OK，但画面黑屏

检查：

1. SDP 是否有正确 `sprop-parameter-sets` 或 `sprop-vps/sps/pps`；
2. PLAY 后是否触发 `ss_mpi_venc_request_idr`；
3. 队列中是否先出现 SPS/VPS；
4. 参数集后是否真的有 IDR/CRA；
5. NAL parser 是否因 3/4 字节起始码错位；
6. 队列满是否丢掉新 IDR；
7. RTP Marker 和 timestamp 是否按 Access Unit 正确；
8. 客户端是否支持当前 H.265 profile/level。

### 27.4 UDP 不播放，TCP 可以

这通常说明编码和 SDP 正常，问题位于传输面：

- 防火墙未开放动态 RTP/RTCP UDP 端口；
- NAT 地址或端口映射错误；
- 客户端 `client_port` 不可达；
- 多网卡时 source/destination address 选择错误；
- 网络丢包过高。

### 27.5 TCP 播放越来越延迟

检查：

- 网络吞吐是否低于编码码率；
- TCP 重传和 RTT；
- RTSP 队列深度是否长期满；
- 是否一直 drop-new、保留旧帧；
- 客户端解码性能；
- 单个慢客户端对共享 RTP sink 的影响；
- socket send buffer 和 live555 输出调度。

### 27.6 有视频无音频

检查：

1. 视频配置是否为 `COMPOSITE_STREAM`；
2. `Audioindex` 是否非空；
3. 音频格式是否映射到 0/1/2/3；
4. AAC 采样率是否属于 8/16/32/48 kHz；
5. SDP 是否存在 audio track；
6. AAC 是否正确去除 7/9 字节 ADTS；
7. G.711 payload 是否与客户端一致；
8. 主、子 audioQueue 是否都收到数据。

### 27.7 有音频但噪声或变速

检查：

- G.711 A-law/μ-law 是否选反；
- 采样率和实际编码数据是否一致；
- AAC AudioSpecificConfig 是否与 AENC 一致；
- G.726 bit width 和 packing；
- RTP timestamp 增量；
- 重采样输出样本数；
- 客户端是否按 SDP clock rate 解码。

### 27.8 修改 live555 源码无效

检查：

```text
源码是否重新编译为 libRtspServer.so
    -> 新 .so 是否复制到 ipc_platform/目标 rootfs
    -> 打包脚本是否包含新库
    -> 目标运行时 ld.so 实际加载路径
    -> md5sum/sha256sum 是否一致
```

## 28. 实用验证命令

### 28.1 ffprobe 查看 RTSP/SDP/track

```bash
ffprobe -loglevel debug \
  -rtsp_transport tcp \
  -show_streams \
  "rtsp://user:password@DEVICE_IP:554/Streaming/Channels/101"
```

重点看：

- 401 后是否成功重试；
- SDP 中 codec、payload、fmtp；
- 视频/音频 time_base；
- 是否持续收到 RTP。

### 28.2 ffplay 强制 TCP

```bash
ffplay -fflags nobuffer -flags low_delay \
  -rtsp_transport tcp \
  "rtsp://user:password@DEVICE_IP:554/Streaming/Channels/101"
```

### 28.3 ffplay 强制 UDP

```bash
ffplay -fflags nobuffer -flags low_delay \
  -rtsp_transport udp \
  "rtsp://user:password@DEVICE_IP:554/Streaming/Channels/101"
```

TCP 正常、UDP 异常时，先查网络和端口，不要先改编码器。

### 28.4 tcpdump

```bash
tcpdump -i any -s 0 -w /tmp/rtsp.pcap \
  'tcp port 554 or udp portrange 6970-7100'
```

实际 UDP 端口以 SETUP 的 Transport 响应为准。

### 28.5 Wireshark 过滤

```text
rtsp
rtp
rtcp
rtcp.pt == 200
rtcp.pt == 201
tcp.analysis.retransmission
```

观察 H.264/H.265 时可检查：

- RTP sequence 是否跳变；
- timestamp 是否按帧增长；
- Marker 是否只在图像末尾；
- FU Start/End 是否成对；
- PLAY 后第一组数据是否包含参数集和 IDR。

## 29. 建议增加的诊断指标

接手后如果要提升可维护性，建议每个 RTSP channel 增加：

- 当前客户端数；
- START/STOP 引用计数；
- video/audio queue current/high watermark；
- push 帧数、pop 帧数；
- drop-new、drop-old；
- SPS/VPS、IDR 请求和实际到达计数；
- IDR 请求到第一个参数集/IDR 的延迟；
- RTP 包数、字节数；
- RTCP RR 丢包率、jitter、RTT；
- event loop 最大回调耗时；
- TCP socket pending bytes；
- RTSP reboot 次数和原因；
- 当前实际加载的 `libRtspServer.so` build id/hash。

没有这些指标时，“网络卡”“编码卡”“队列卡”“客户端解码卡”很容易混在一起。

## 30. 推荐的接手阅读顺序

第一遍只理解主链：

```text
stream_main.cpp
 -> stream_video.cpp / stream_audio.cpp
 -> venc_channel_handler.cpp
 -> push_stream.cpp
 -> rtsp_server.cpp
 -> rtspServer_base.cpp
```

第二遍理解 RTSP 会话：

```text
RTSPServer.cpp
 -> ServerMediaSession.cpp
 -> OnDemandServerMediaSubsession.cpp
```

第三遍按 codec 阅读：

```text
h264/h265/mjpeg/aac/g711/g726 subsession
 -> 对应 FramedSource
 -> 对应 RTPSink
```

第四遍理解网络与同步：

```text
MultiFramedRTPSink.cpp
 -> H264or5VideoRTPSink.cpp
 -> RTPInterface.cpp
 -> RTCP.cpp
```

最后再处理风险点和配置重启。这样不会一开始陷入 live555 大量通用代码，而忽略项目真正的回调边界。

## 31. 一条完整调用链速查

### 31.1 客户端建立主码流

```text
Client TCP connect :554
 -> RTSPServer::createNewClientConnection
 -> OPTIONS
 -> DESCRIBE /Streaming/Channels/101
 -> Digest authenticationOK
 -> ServerMediaSession::generateSDPDescription
 -> H264/H265 getAuxSDPLine
 -> dummy source 通过 rtspFrameCall 取参数集
 -> SETUP track1 / track2
 -> getStreamParameters
 -> 创建或复用 StreamState
 -> PLAY
 -> startStream
 -> RTSPCLIENT_START
 -> request=1, requestIFrame=1
 -> ss_mpi_venc_request_idr
```

### 31.2 主码流数据发送

```text
ss_mpi_venc_get_stream
 -> ot_venc_stream.pack[i]
 -> CStreamVideo::createFrame
 -> CMainChannelHandler::handleFrame
 -> CPushStream::sendVideoData
 -> CRtspServer::sendVideoData(MAIN)
 -> FrameData 入 videoQueue
 -> H264/H265 Video Source::getNextFrame1
 -> rtspFrameCall
 -> 等 SPS/VPS 起点
 -> memcpy 到 fTo
 -> H264/H265 StreamFramer
 -> H264/H265 RTP Sink
 -> 单 NAL 或 FU 分片
 -> RTPInterface
 -> UDP socket 或 RTSP TCP interleaved socket
```

### 31.3 音频发送

```text
AI/AENC
 -> AAC: 去 ADTS；G711/G726: 对应编码帧
 -> AudioFrame_S
 -> CPushStream::sendAudioData
 -> 主、子 RTSP audioQueue 各复制一份
 -> AAC/G711/G726 Audio Source
 -> rtspFrameCall
 -> 对应 RTP Sink
 -> RTP/RTCP
```

## 32. 最终接手结论

这个项目的 RTSP 核心不是一个单独的“推流函数”，而是四个系统的交界：

1. 海思 MPP 提供编码数据和 IDR 控制；
2. IPC 业务层负责配置、路由、队列、生命周期和跨线程所有权；
3. 项目自定义 mediaServer 层把回调适配成 live555 source/subsession；
4. live555 负责 RTSP、SDP、RTP、RTCP、Digest 与 UDP/TCP 网络发送。

以后排查问题时先判断故障属于哪一面：

```text
控制面：RTSP 方法、鉴权、Session、URL、SETUP
描述面：SDP、payload、fmtp、参数集
媒体面：VENC/AENC、队列、NAL/AU、IDR
传输面：RTP sequence/timestamp、UDP/TCP、RTCP、丢包
生命周期：init/deinit/reboot、线程退出、内存所有权
```

只要先完成这个分类，再沿本文给出的调用链定位，绝大多数“连不上、黑屏、无音频、卡顿、延迟累积、重启崩溃”问题都能收敛到有限的函数和状态变量，而不需要在整个工程里盲目搜索。

# 第二部分：源码逐函数、逐代码块导读

前面的章节回答“系统由哪些部分组成、数据怎么流动”。从本章开始进入代码阅读模式：不再只列文件，而是把关键函数的真实代码拆开，说明每条语句改变了什么状态、运行在哪个线程、为什么需要这样写，以及当前实现存在哪些隐含前提。

本文引用的行号以当前工作区为准。后续源码增删会导致行号漂移，因此定位时应同时搜索函数名。

## 33. 源码覆盖矩阵与阅读规则

### 33.1 必须读懂的 IPC 上层函数

| 模块 | 函数 | 必须理解的内容 |
|---|---|---|
| 程序入口 | `main()` | 音视频与 RTSP 初始化、退出顺序 |
| 视频 | `CStreamVideo::init()` | VI/VPSS/VENC 建立、线程启动、回调注册 |
| 视频 | `CStreamVideo::get_vencStream()` | MPP 取流、pack 生命周期、分发与释放 |
| 视频 | `createFrame()/freeFrame()` | Annex-B 数据复制、NAL 识别、分配器问题 |
| 视频 | `request_idr()` | RTSP 到 MPP 请求关键帧的最终落点 |
| 路由 | `CMainChannelHandler::handleFrame()` | 主码流 RTSP/GB28181/录像分流 |
| 路由 | `CSubChannelHandler::handleFrame()` | 子码流 RTSP/录像分流 |
| 推流门面 | `CPushStream::init()/deinit()` | RTSP enable 配置和服务生命周期 |
| 推流门面 | `sendVideoData()/sendAudioData()` | 主子通道映射与音频广播 |
| RTSP 业务层 | `CRtspServer::init()/deinit()/reboot()` | session、队列、live555 句柄的生命周期 |
| RTSP 业务层 | `sendVideoData()/sendAudioData()` | 按需入队、深拷贝和丢帧策略 |
| 回调 | `rtspStateCallback()` | 客户端状态如何驱动队列与 IDR |
| 回调 | `rtspFrameCall()` | live555 如何反向从业务队列取帧 |
| IDR | `triggerRequestIdr()` | 频控、初始化门控和跨模块回调 |
| 队列 | `CThreadSafeFrameQueue` 全部方法 | 生产消费、所有权、满队列行为 |
| 音频 | `deal_aencFrame_thr()` | AAC ADTS 去头、AudioFrame 创建与发送 |

### 33.2 逐行讲解的四个观察维度

看到一行代码时，不只问“它做什么”，还要问：

1. **所有权**：这块内存、socket、frame 或 session 当前归谁？何时失效？
2. **线程**：这行在哪个线程执行？另一线程能否同时修改同一对象？
3. **协议状态**：它对应 RTSP 的 DESCRIBE、SETUP、PLAY，还是 RTP 发送状态？
4. **失败路径**：返回失败后已经创建的资源是否回滚？上层是否真正检查返回值？

后续解释均按这四个维度展开。

## 34. `stream_main.cpp`：为什么必须先音视频、后 RTSP

源码位置：`Hi3516/hi3516_ipc/main_app/stream_main.cpp:74-119`。

### 34.1 初始化代码

```cpp
nRet = CConfigManager::instance()->init();
if (nRet < OK)
{
    dlog_error("初始化全局配置失败：%d", nRet);
    goto exit_perf_monitor;
}

nRet = CCryptoInit::instance()->init();
if (nRet < OK)
{
    dlog_error("密码学模块初始化失败：%d", nRet);
    goto exit_crypto_init;
}

nRet = CStreamVideo::instance()->init();
if (nRet < OK)
{
    dlog_error("视频模块初始化失败：%d", nRet);
    goto exit_stream_video;
}

nRet = CStreamAudio::instance()->init();
if (nRet < OK)
{
    dlog_error("音频模块初始化失败：%d", nRet);
    goto exit_stream_audio;
}

nRet = CPushStream::instance()->init();
if (nRet < OK)
{
    dlog_error("推流模块初始化失败：%d", nRet);
    goto exit_push_stream;
}
```

逐句解释：

- `CConfigManager::instance()->init()` 必须最先执行，因为视频 codec、帧率、音频格式、RTSP 开关、端口和认证算法都来自配置。RTSP 如果先启动，只能使用成员默认值，之后再改配置就必须额外 reboot。
- `nRet < OK` 而不是 `nRet != OK`，说明工程约定负数为错误，0 为成功，部分接口可能返回非负扩展值。
- `goto exit_xxx` 不是跳到当前模块的清理，而是跳入函数尾部按逆序执行已经成功模块的 deinit。嵌入式 C/C++ 项目常用这种单出口回滚，避免每个失败分支重复写一长串释放代码。
- Crypto 在视频前初始化，是因为后续 HTTPS、Digest、安全模块或硬件加密可能共享 OpenSSL/provider 环境。
- `CStreamVideo::init()` 在 RTSP 之前，是因为 live555 的 DESCRIBE 阶段就可能创建 dummy source 并请求 SPS/PPS/VPS。若 VENC 尚未创建，IDR 回调会落到空 handle。
- `CStreamAudio::init()` 同样在 RTSP 之前，保证复合流创建 audio subsession 后，AENC 数据源已经具备生产能力。
- `CPushStream::init()` 是 RTSP/RTMP 的统一门面。RTSP 服务器到这里才监听端口，因此客户端不可能在 VENC 建立前正常进入 PLAY。

这里的核心不变量是：

```text
RTSP 可接受客户端
    => 视频模块已初始化
    => 音频模块已初始化
    => IDR 回调目标有效
```

### 34.2 为什么退出顺序相反

源码位置：`stream_main.cpp:217-233`。

```cpp
nRet = CPushStream::instance()->deinit();
nRet = CStreamAudio::instance()->deinit();
nRet = CStreamVideo::instance()->deinit();
```

- 先停 `CPushStream`，使 RTSP 不再接受新客户端，并停止 live555 source 继续调用 `rtspFrameCall()`。
- 再停音频，防止 AENC 线程仍向已经销毁的 RTSP `audioQueue` push。
- 最后停视频，防止 live555 在销毁 session 期间请求 IDR 时访问已经销毁的 VENC handle。
- 源码注释“需提前音视频模块停止”文字容易误解；真实代码是先停止推流模块，再停止音视频生产者。这里“提前”表达的是推流关闭动作要放在音视频对象销毁之前。

如果顺序写成视频先 deinit，会出现如下竞争：

```text
线程 A：CStreamVideo::deinit -> free VENC handle
线程 B：RTSP PLAY -> triggerRequestIdr -> request_idr -> 使用已释放 handle
```

## 35. `CStreamVideo::get_vencStream()`：MPP 编码帧如何进入 RTSP

源码位置：`stream_video.cpp:819-925`。

这个函数运行在 VENC 取流线程，而不是 live555 event loop。它是 MPP 世界与网络推流世界的生产者边界。

### 35.1 取得 VENC handle

函数首先根据线程参数确定通道：

```cpp
void CStreamVideo::get_vencStream(int param)
{
    int nChannel = param;
    HiVenc_S* pHandle = m_vencHandles[nChannel];
```

- `param` 是线程启动时传入的通道号，主码流、子码流分别启动自己的取流执行单元。
- `m_vencHandles[nChannel]` 是项目对海思 VENC channel 的封装，其中函数指针最终调用 `ss_mpi_venc_*`。
- 这里取的是裸指针，因此函数存活期间依赖 `CStreamVideo::deinit()` 不会并发释放该 handle。退出顺序和线程 join 必须保证这一点。

### 35.2 从 MPP 获取 `ot_venc_stream`

核心循环的语义是：

```text
等待编码器有输出
    -> get_stream
    -> 遍历 pack
    -> 分发
    -> release_stream
```

`ot_venc_stream` 本身不拥有一份业务层独立内存。`pack[i].addr` 指向 VENC 管理的码流缓冲，只有从 `get_stream` 成功到 `release_stream` 之间有效。

因此以下代码顺序不能交换：

```cpp
pData = stFrame.pack[i].addr + stFrame.pack[i].offset;
nDataLen = stFrame.pack[i].len - stFrame.pack[i].offset;
```

- `addr` 是 pack 缓冲起始地址。
- `offset` 表示当前有效码流不是从缓冲第 0 字节开始；可能前面有无效区或已消费区。
- `len` 是 pack 总长度，因此有效长度必须减去 offset。
- 如果直接使用 `len`，会把 offset 前的无效字节也送入 Annex-B parser，NAL 起始码可能无法识别。

### 35.3 为什么逐 pack，而不是把 `pack_cnt` 合并

```cpp
for (int i = 0; i < (int)stFrame.pack_cnt; i++)
{
    pData = stFrame.pack[i].addr + stFrame.pack[i].offset;
    nDataLen = stFrame.pack[i].len - stFrame.pack[i].offset;
```

海思一次 `get_stream` 返回的编码图像可能包含多个 pack，例如参数集、SEI、slice 等。项目选择逐 pack 分发，优点是：

- 不需要先计算总长度并再分配一次大缓冲；
- 可以立即识别每个 pack 开头的 NAL 类型；
- 参数集能独立进入队列。

代价是：

- 队列中的一项不一定是一幅完整图像，而可能只是一个 NAL 或一段码流；
- `fDurationInMicroseconds` 不能简单地对每一项都加一帧时长；
- 队列满时可能只丢掉一幅图像中的部分 NAL，造成该 Access Unit 不完整。

### 35.4 负长度检查为什么仍有缺口

```cpp
if (nDataLen < 0)
{
    throw std::runtime_error("stFrame nDataLen < 0 !");
}
```

- 该检查防止 `offset > len` 时把负数转换成很大的无符号长度并触发超大 memcpy。
- 但它没有排除 `nDataLen == 0`。后续 `createFrame()` 会因为长度小于 6 返回 null。
- 在常驻采集线程中抛出异常风险较高：如果线程入口没有 catch，整个进程可能 `std::terminate`。更稳妥的策略是记录 pack 元数据、释放本次 stream 并继续或触发受控重启。

### 35.5 创建业务层帧

```cpp
Video_NS::VideoFrame_S* pVideoFrame = nullptr;
if (nChannel != VENC_CHN_JPEG)
{
    pVideoFrame = createFrame(static_cast<VENC_CHN_E>(nChannel), pData, nDataLen);
}
```

- JPEG 抓图通道走独立处理器，所以不创建用于 RTSP/GB28181 的 `VideoFrame_S`。
- 主码流、子码流、MJPEG 视频通道通过 `createFrame()` 做深拷贝。深拷贝是必要的，因为当前 MPP pack 很快会被 release。
- `static_cast<VENC_CHN_E>` 把整数通道恢复成强语义枚举，供配置索引和 codec parser 使用。

### 35.6 策略处理器分流

```cpp
m_channelHandlers[nChannel]->handleFrame(
    pData, nDataLen, pVideoFrame, m_configManager, nChannel);
```

这里采用策略对象而不是在大循环里写一串 `if (main) ... else if (sub)`：

- VENC 获取和释放逻辑只写一次；
- 主码流、子码流、JPEG 的业务路由分别封装；
- 后续增加新通道时不必改动 MPP 取流循环的核心资源生命周期。

`pData/nDataLen` 与 `pVideoFrame` 同时传入，是因为：

- RTSP、GB28181 等需要带 codec/NAL 元数据的 `VideoFrame_S`；
- JPEG 抓图等路径可能直接使用原 pack 字节。

### 35.7 为什么必须在 `release_stream` 前完成所有同步复制

```cpp
if (pVideoFrame)
{
    freeFrame(pVideoFrame);
}
...
pHandle->mppVenc_release_stream(pHandle, &stFrame);
```

此时各下游必须已经完成自己的深拷贝：

- `CRtspServer::sendVideoData()` 复制到 `FrameData`；
- GB28181/录像模块也必须遵循各自所有权契约。

`release_stream` 后，`pData` 和 `stFrame.pack[i].addr` 都不能再使用。项目没有把 MPP pack 裸指针放进 RTSP 队列，这是正确的生命周期隔离。

### 35.8 `usleep(1000)` 的真实作用

```cpp
usleep(1000);
```

它让当前线程主动让出约 1 ms CPU，避免编码器持续有数据时形成完全无间隙的忙循环。它不是帧率控制：帧率由 VI/VPSS/VENC 决定。固定 sleep 也会增加一点调度延迟，是否保留应由 CPU 占用和端到端延迟数据决定。

## 36. `createFrame()` 与 NAL 解析：为什么复制，哪里有未定义行为

源码位置：`stream_video.cpp:770-814`。

### 36.1 参数校验

```cpp
if (!pData || nDataLen < 6)
{
    dlog_error("传入参数不正确");
    return nullptr;
}
```

- 空指针检查防止 memcpy 崩溃。
- 长度 6 是为了至少容纳 4 字节起始码、NAL header 和一个数据字节。
- H.264 只解析到 `pData[4]`，理论最小 5；这里取 6 是更保守的业务门槛。
- 这个检查同时隐含“输入使用 4 字节起始码”。如果是 3 字节起始码，长度够也会解析错位置。

### 36.2 连续分配的目的

```cpp
Video_NS::VideoFrame_S* pVideoFrame =
    (Video_NS::VideoFrame_S*)malloc(sizeof(Video_NS::VideoFrame_S) + nDataLen);
```

`VideoFrame_S` 使用柔性尾部数组风格：结构体元数据后面紧跟 `pData` 字节。一次分配的好处是：

- 元数据和数据一起释放；
- 减少一次独立 heap allocation；
- cache locality 更好；
- 传给 C 风格接口方便。

这也是为什么不能简单改成 `new VideoFrame_S`：普通 `new` 只分配固定结构体大小，不包含尾部 `nDataLen`。

### 36.3 复制和元数据填写

```cpp
memcpy(pVideoFrame->pData, pData, nDataLen);
pVideoFrame->nLen = nDataLen;
pVideoFrame->enVideoCodec = videoConfig.enVideoCodec;
```

- memcpy 使 `VideoFrame_S` 脱离 MPP pack 生命周期。
- `nLen` 后续是 RTSP 队列再次分配和 memcpy 的唯一长度依据，若这里被破坏会直接造成越界。
- codec 不能只从 NAL 字节猜，因为 H.264/H.265/MJPEG 解析规则不同；它来自当前 VENC channel 配置。

### 36.4 策略表解析 NAL 类型

```cpp
auto it = m_nalParsers.find(videoConfig.enVideoCodec);
if (it != m_nalParsers.end() && it->second)
{
    pVideoFrame->eType = it->second->parseNalType(pData, nDataLen);
}
```

- `m_nalParsers` 把 codec 枚举映射到解析策略，避免在 `createFrame()` 内写 codec switch。
- 同时检查 `it != end()` 和 `it->second`，分别防止“没有注册该 codec”和“注册值是空指针”。
- `eType` 不是 live555 封包所必需；当前主要被 RTSP 用来识别 SPS/VPS 起播边界，也供其他协议判断 IDR/参数集。

H.264 parser：

```cpp
uint8_t nalType = pData[4] & 0x1F;
```

- `pData[0..3]` 被假定为 `00 00 00 01`；
- 第 5 字节是 H.264 NAL header；
- 低 5 位是 nal_unit_type；
- 7 为 SPS、8 为 PPS、5 为 IDR。

H.265 parser：

```cpp
uint8_t nalType = (pData[4] >> 1) & 0x3F;
```

- H.265 NAL header 有 2 字节；
- type 位于第一个 header 字节的 bit 1..6；
- 32/33/34 分别是 VPS/SPS/PPS；
- 19/20 是两种 IDR。

### 36.5 找不到 parser 时的释放错误

```cpp
else
{
    dlog_error("未找到对应的NAL解析器...");
    delete pVideoFrame;
    return nullptr;
}
```

`pVideoFrame` 来自 `malloc`，这里却使用 `delete`，分配器不匹配。正常路径的 `freeFrame()` 同样使用 `delete`：

```cpp
void CStreamVideo::freeFrame(VideoFrame_S* pVideoFrame)
{
    if (pVideoFrame)
    {
        delete pVideoFrame;
    }
}
```

这不是代码风格问题，而是 C++ 未定义行为。堆分配器的元数据布局可能不同，后果包括：

- 当场崩溃；
- 延迟到之后一次 malloc/free 才暴露；
- 只在压力或特定 libc 版本出现；
- 表面表现为 RTSP 随机断流，实际是 heap corruption。

在保持柔性数组设计时，配对释放必须是 `free(pVideoFrame)`。

## 37. 主、子码流处理器：同一编码帧为什么会走多个协议

### 37.1 主码流

源码位置：`venc_channel_handler.cpp:31-66`。

```cpp
const auto& videoConfig = configManager.getVideoConfigs().at(nChannel);
```

- 使用 `const auto&` 避免复制整个配置结构。
- `.at()` 而非 `operator[]` 会在通道越界时抛出异常，能暴露内部索引错误，但线程入口必须有异常边界才能避免进程退出。

```cpp
if (videoConfig.enVideoCodec != Video_NS::VideoCodec_E::SVAC3)
{
    CPushStream::instance()->sendVideoData(pVideoFrame, true, true);
}
```

- live555 适配层只创建 H.264、H.265、MJPEG subsession，没有 SVAC3 RTP sink。
- 因此主码流在业务入口就过滤 SVAC3，避免把无法解释的码流送进一个错误的 H.264/H.265 session。
- 两个 `true` 分别表示主码流和 RTSP。这个布尔接口可读性弱，调用点需要结合函数签名才能理解。

```cpp
SIP::CRtpServer::instance()->sendVideoData(pVideoFrame);
```

这条链属于 GB28181/SIP，不经过 live555 RTSP。两者共享同一 `VideoFrame_S`，所以调用必须在 `freeFrame()` 之前同步完成深拷贝或消费。

录像宏打开时同一帧还会进入 `CStreamServer`。因此一份 MPP pack 会扇出到多个协议，但每个异步消费者都必须建立自己的所有权副本。

### 37.2 子码流

源码位置：`venc_channel_handler.cpp:99-125`。

```cpp
CPushStream::instance()->sendVideoData(pVideoFrame, false, true);
```

- `false` 映射到 `RTSP_CHN_SUB`；
- `true` 表示目标包含 RTSP；
- 子码流不发送到 GB28181 的这条默认调用链。

当前子码流没有主码流的 SVAC3 检查。如果配置层允许子码流设成 SVAC3，`CRtspServer::init()` 的 codec switch 不会设置合法 `nProtolType`，而数据仍持续进入子码流 RTSP 入口。这是主子实现不对称，需要配置约束或代码补齐。

## 38. `CPushStream`：它是协议门面，不是 RTP 实现

源码位置：`Hi3516/share/ipc_share/push_stream/push_stream.cpp`。

### 38.1 `init()` 的 RTSP 部分

```cpp
Network::HttpsConfigInfo_S stInfo;
if (Convert::read_file(m_strHttpsConfigFile, stInfo))
{
    Convert::write_file(m_strHttpsConfigFile, stInfo);
}
```

- RTSP enable 被放在 `HttpsConfigInfo_S` 中，这是历史配置聚合，不代表 RTSP 依赖 HTTPS。
- 工程中的 `read_file()` 返回语义与常见习惯相反：进入 if 表示读取失败或文件不存在，随后把默认结构写回。
- 接手时不能仅凭函数名推断返回值，必须查看 Convert 封装约定。

```cpp
if (stInfo.bEnRtsp && !CRtspServer::instance()->isInit())
{
    CRtspServer::instance()->init();
}
```

- 第一个条件是持久化开关；第二个条件保证 init 幂等，避免重复 bind 554 端口。
- 这里没有保存或检查 `CRtspServer::init()` 返回值。即使端口绑定失败，函数后面仍会把 `CPushStream::m_bInitFlag=true` 并返回 OK。
- 因此日志可能显示推流模块初始化成功，但 RTSP 实际未监听。排障时必须看 `CRtspServer` 自己的 init 日志和 socket 状态。

### 38.2 视频路由函数

```cpp
if (m_bInitFlag == true)
{
    if (bIsRtsp == true)
    {
        if (bIsMain == true)
        {
            nRet = CRtspServer::instance()->sendVideoData(RTSP_CHN_MAIN, pVideoFrame);
        }
        else
        {
            nRet = CRtspServer::instance()->sendVideoData(RTSP_CHN_SUB, pVideoFrame);
        }
    }
}
```

逐层门控：

- `m_bInitFlag` 表示门面模块完成 init，不等价于 RTSP 实例一定成功；
- `bIsRtsp` 允许调用者只走其他协议；
- `bIsMain` 完成业务通道到 RTSP 0/1 通道的映射；
- `pVideoFrame` 在函数入口检查非空，但没有检查 `nLen` 和 codec 与当前 session 的一致性，这些由上游配置不变量保证。

这个函数没有做 RTP 封包，也没有调用 FFmpeg。它只把帧交给 `CRtspServer`。

### 38.3 为什么一帧音频要复制到主、子两路

```cpp
nRet = CRtspServer::instance()->sendAudioData(RTSP_CHN_MAIN, pAudioFrame);
nRet = CRtspServer::instance()->sendAudioData(RTSP_CHN_SUB, pAudioFrame);
```

IPC 只有一条公共音频编码链，但主码流 `/101` 和子码流 `/102` 是两个独立 `ServerMediaSession`，各自有 audio subsession 和 audio queue。因此同一个 AAC/G.711/G.726 帧必须分别进入两个 session。

第二次赋值覆盖第一次 `nRet`：如果主路失败而子路成功，最终返回成功；反之只看到子路失败。若调用者依赖返回值做告警，应分别保存两个返回码再合并。

### 38.4 `deinit()` 为什么先判断实例状态

```cpp
if (CRtspServer::instance()->isInit())
{
    CRtspServer::instance()->deinit();
}
```

- 防止在未成功创建 `m_pLiveInfo` 时进入无空指针保护的 deinit。
- 但如果 `CRtspServer::init()` 中途失败、还未设置 `m_bInitFlag`，已经分配的部分资源不会由这里释放。这要求 init 自己具备完整失败回滚；当前实现还不完整。

## 39. `CStreamVideo::init()/deinit()`：RTSP 能工作的硬件前置条件

源码位置：`stream_video.cpp:78-254`。

### 39.1 先创建策略对象

```cpp
m_channelHandlers[VENC_CHN_MAIN] =
    std::make_unique<CMainChannelHandler>(this);
m_channelHandlers[VENC_CHN_SUB] =
    std::make_unique<CSubChannelHandler>(this);
m_channelHandlers[VENC_CHN_JPEG] =
    std::make_unique<CJpegChannelHandler>();
```

- handler 使用 `unique_ptr`，生命周期归 `CStreamVideo`，不需要手动 delete。
- 主、子 handler 保存 `this`，因为录像定时请求 IDR 等操作要回调 `CStreamVideo::request_idr()`。
- JPEG handler 不需要访问视频主对象，所以不传 `this`。
- 这些对象必须在 VENC 线程启动前创建，否则线程可能取得第一帧后解引用空 handler。

NAL parser 也在启动线程前建立：

```cpp
m_nalParsers[H264] = std::make_unique<CH264NalParser>();
m_nalParsers[H265] = std::make_unique<CH265NalParser>();
m_nalParsers[SVAC3] = std::make_unique<CSvac3NalParser>();
m_nalParsers[MJPEG] = std::make_unique<CMjpegParser>();
```

这里注册 SVAC3 parser 是因为 GB28181/其他业务可能使用它；注册不代表 live555 支持 SVAC3。

### 39.2 IDR 回调为什么在 MPP 初始化前注册

```cpp
initCallbackBinding();
```

注册动作只保存一个 `std::function`，不会立即请求 IDR，所以放在 MPP 初始化前是安全的。这样可以保证无论后续哪一步完成后 RTSP 启动，回调关系已经存在。

实际绑定代码：

```cpp
CRtspServer::instance()->setRequestIdrCallback(
    [this](int nChannel, void* pUserData) -> void
    {
        return this->request_idr(nChannel);
    },
    &m_bInitFlag);
```

逐项解释：

- `[this]` 捕获当前 `CStreamVideo` 单例，使 lambda 能调用成员函数。
- `pUserData` 形参没有在 lambda 中使用，它只是匹配统一回调签名。
- `return this->request_idr(...)` 对 void 函数来说合法，但写成直接调用更清晰。
- 第二个参数传入 `m_bInitFlag` 地址，RTSP 在触发前解引用它，避免视频模块尚未完成 init 时调用 MPP。
- 这是跨模块裸指针。它依赖 `CStreamVideo` 是进程级单例、地址永久稳定。
- 如果 `m_bInitFlag` 是普通 bool，RTSP event loop 与初始化/反初始化线程并发读写仍缺少原子同步。

### 39.3 为什么先 `streamSys_init()`

```cpp
const auto& videoConfigs = m_configManager.getVideoConfigs();
nRet = streamSys_init(videoConfigs);
```

`streamSys_init()` 会依据各路分辨率和像素格式计算 VB block，并初始化 MPP SYS/VB。VI、VPSS、VENC 随后申请的图像缓冲都依赖 SYS/VB 已就绪。

虽然 RTSP 最终消费的是编码数据，但编码数据的产生依赖完整硬件链：

```text
SYS/VB -> VI/ISP -> VPSS -> VENC -> RTSP
```

所以 RTSP 故障有时根因在 VB 配置不足，而不是网络模块。

### 39.4 VI、ISP、VPSS 的顺序

```cpp
m_viHandle.reset(streamVi_init());
CIspControl::instance()->init();
streamVpss_init(&m_pVpssHandle, videoConfigs);
```

- VI handle 用 RAII wrapper 保存，`reset(pointer)` 接管 C 接口返回的资源。
- ISP 依赖 sensor/VI 管线存在，用于曝光、白平衡、日夜切换等图像控制。
- VPSS 在 VI 之后创建，负责从公共输入生成主码流、子码流和 AI/JPEG 尺寸的图像。
- `streamVpss_init()` 返回值在这里没有检查，后面直接解引用 `m_pVpssHandle[...]`。如果初始化失败，可能不是优雅返回而是空指针崩溃。

### 39.5 VENC 创建后为何立即启动取流线程

```cpp
m_vencHandles[i].reset(streamVenc_init(...));
if (!m_vencHandles[i])
{
    return ERR;
}
m_bVencFlag[i].store(true, std::memory_order_release);
m_getVencThread[i] =
    std::thread(&CStreamVideo::get_vencStream, this, i);
```

- handle 成功后才设置运行标志，防止线程看到 true 却没有有效 VENC。
- `memory_order_release` 保证当前线程在 store 之前完成的初始化写入，对之后 acquire 读取该标志的线程可见。需要核对循环读取是否使用 acquire；只在写侧使用 release 不能单独建立同步。
- 线程对象保存在数组中，没有 detach，意味着 deinit 可以 join，资源生命周期更可控。
- 线程启动发生在 `bindModule()` 之前。线程可能先进入 get_stream 等待；直到 VPSS→VENC 绑定并有输入后才得到数据。这种顺序可行，但 init 失败回滚更复杂，因为线程已经运行。

### 39.6 `bindModule()` 的每一条边

```cpp
nRet |= mppVi_bind_vpss(viDev, pipeId, VPSS_MAIN_SUB, 0);
nRet |= mppVpss_bind_venc(group, VPSS_CHANNEL_MAIN, VENC_CHN_MAIN);
nRet |= mppVpss_bind_venc(group, VPSS_CHANNEL_SUB, VENC_CHN_SUB);
nRet |= mppVpss_bind_venc(group, VPSS_CHANNEL_AI, VENC_CHN_JPEG);
```

它建立硬件数据流：

```text
VI pipe -> VPSS group input
VPSS main -> VENC main
VPSS sub  -> VENC sub
VPSS AI   -> VENC JPEG
```

`nRet |=` 允许尝试所有绑定并合并错误，但有两个缺点：

- 无法知道是哪一条边失败；
- 不同错误码按位或后可能变成不存在的组合码。

更适合排障的写法是逐项保存返回值并附带源/目的模块日志。

### 39.7 为什么 `m_bInitFlag=true` 放在 bind 成功后

```cpp
m_bInitFlag = true;
```

RTSP 的 IDR 回调把它当作“VENC 可安全调用”的最终门。只有 SYS、VI、ISP、VPSS、VENC、线程和绑定都成功后才置 true，意图正确。

但 init 中途失败时已经创建的线程和 handle 没有统一 rollback。因为 `m_bInitFlag` 仍是 false，外层可能不调用 deinit，导致部分资源残留。

### 39.8 deinit 为什么先解绑再停线程

```cpp
m_bInitFlag = false;
unbindModule();
m_bVpssFlag[AI].store(false);
join(vpssThread);
...
m_bVencFlag[i].store(false);
join(vencThread[i]);
m_vencHandles[i].reset();
```

- 先把 init flag 设 false，阻止新的 RTSP IDR 回调。
- 先解绑硬件链，停止新的图像继续流向 VENC。
- 再把线程循环标志设 false并 join，确保没有线程仍使用 handle。
- 最后 reset VENC handle，顺序满足“使用者先退出、资源后销毁”。

潜在问题是：如果 `mppVenc_get_stream` 是无限阻塞，而解绑后没有数据唤醒，join 可能卡住。需要看 MPP wrapper 是否带 timeout 或 stop channel 能唤醒。

## 40. `request_idr()`：RTSP 请求如何真正进入海思 VENC

源码位置：`stream_video.cpp:322-335`。

```cpp
td_bool bInstant = TD_TRUE;
```

海思接口的 `instant` 参数表示立即插入 IDR，而不是等常规 GOP 边界。新 RTSP 客户端希望尽快出图，所以选择 true。

```cpp
if (m_vencHandles[nChannel])
{
    nRet = m_vencHandles[nChannel]->mppVenc_request_idr(
        m_vencHandles[nChannel].get(), bInstant);
}
```

- 先检查 handle 智能包装是否有效，避免空指针。
- `mppVenc_request_idr` 是 handle 中的函数指针，第一参数再次传 handle，保持 C 风格对象接口。
- wrapper 最终调用 `ss_mpi_venc_request_idr(chn, TD_TRUE)`。
- 这里没有校验 `nChannel` 范围；调用链依赖 RTSP 只传 0/1。
- 请求成功只表示命令已提交，不表示下一次取流立刻就是 SPS/VPS/IDR。编码器还需要完成当前图像并输出参数集。

因此衡量起播性能应该记录两个时间：

```text
t0 = request_idr 调用成功
t1 = 队列观察到 SPS/VPS
t2 = 队列观察到 IDR slice
```

只记录接口返回 0 无法证明客户端拿到了可解码随机访问点。

## 41. 音频 AENC 线程：AAC 为什么必须去掉 ADTS

源码位置：`stream_audio.cpp:855-930`。

### 41.1 识别 AAC 编码通道

```cpp
if (AENC_AAC_CHN == nChannel)
```

音频模块可能同时管理 AAC、G.711 或解码通道。只有 AAC AENC 输出带 ADTS，因此去头逻辑不能作用到 G.711/G.726。

### 41.2 同步字检查

```cpp
if ((stFrame.stream[0] != 0xFF) ||
    ((stFrame.stream[1] & 0xF0) != 0xF0))
{
    mppAenc_releaseFrame(...);
    continue;
}
```

- ADTS syncword 是 12 个 1，即 `0xFFF`。
- 第一个字节必须 0xFF；第二字节高 4 位必须 0xF。
- 使用逻辑 OR：任一部分不满足就判定不是 ADTS。
- 失败后必须先 release AENC frame，再 continue，否则海思编码缓冲会泄漏并最终停止出帧。
- 代码访问 `[0]`、`[1]` 前没有验证 `stFrame.len >= 2`，依赖 AENC 永远返回合法非短帧。

### 41.3 7 字节还是 9 字节

```cpp
if (!(stFrame.stream[1] & 0x01))
{
    nADTSLen = 9;
}
else
{
    nADTSLen = 7;
}
```

ADTS `protection_absent` 位：

- 1：没有 CRC，固定头 7 字节；
- 0：后面还有 16-bit CRC，总头长 9 字节。

变量名 `nADTSLen` 实际表示“要跳过的 ADTS header length”。

### 41.4 创建裸 AAC frame

```cpp
AudioFrame_S* pAudioFrame = createFrame(
    stFrame.stream + nADTSLen,
    stFrame.len - nADTSLen);
```

- 指针前移跳过 ADTS；
- 长度同步减去 header；
- `createFrame` 再做一次深拷贝，使数据能在 AENC frame release 后继续被 RTSP 使用。

为什么 RTSP 不要 ADTS：AAC RTP 使用 MPEG4-GENERIC，采样率、声道和 object type 在 SDP `config` 中描述，RTP payload 只放 AAC Access Unit 和 AU Header。若保留 ADTS，客户端会把 header 当音频压缩数据，造成解码错误。

### 41.5 推流与录像共享同一个临时 frame

```cpp
CPushStream::instance()->sendAudioData(pAudioFrame);
CStreamServer::instance()->sendAudioData(pAudioFrame);
freeFrame(pAudioFrame);
```

- `pAudioFrame` 由当前 AENC 线程拥有；
- RTSP 和录像接口若异步使用，必须在返回前完成深拷贝；
- 最后当前线程释放临时 frame。

音频 `createFrame()` 使用 malloc，而 `freeFrame()` 使用 delete，与视频相同，是未定义行为。

## 42. `CThreadSafeFrameQueue`：每个方法的同步与所有权语义

源码位置：`push_stream/common/frame_queue.h`。

### 42.1 `FrameData` 为什么禁用拷贝

```cpp
struct FrameData
{
    std::unique_ptr<unsigned char[]> data;
    int frameSize = 0;
    int type = 0;
    int iFrame = 0;

    FrameData(const FrameData&) = delete;
    FrameData& operator=(const FrameData&) = delete;
    FrameData(FrameData&&) = default;
    FrameData& operator=(FrameData&&) = default;
};
```

- `data` 独占字节数组，因此一个 frame 只能有一个所有者。
- 禁止拷贝避免无意中出现双重所有权或额外大内存复制。
- 允许移动，使 frame 能从生产线程转移到 deque，再转移到消费线程。
- `iFrame` 名称不精确：H.264 存 SPS 起点，H.265 存 VPS 起点。

### 42.2 `push()`

```cpp
bool push(std::unique_ptr<FrameData> frame)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.size() >= m_maxSize)
        {
            return false;
        }
        m_queue.push_back(std::move(frame));
    }
    m_cv.notify_one();
    return true;
}
```

逐行语义：

- 参数按值接收 `unique_ptr`，调用者必须 `std::move(frame)`，所有权在进入函数时已经转交。
- mutex 同时保护 size 检查和 push，避免两个生产者都看到“还有一个空位”后把容量超出限制。
- 队列满立即返回，不阻塞 VENC/AENC 生产线程。实时编码线程不能因为一个网络慢客户端无限等待。
- 返回 false 时，形参 unique_ptr 离开函数并自动释放该新帧，所以策略是 drop-new。
- `notify_one()` 放在解锁后，避免被唤醒线程马上又阻塞在同一 mutex。

### 42.3 `pop()`

```cpp
std::unique_ptr<FrameData> pop()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_queue.empty())
    {
        return nullptr;
    }
    auto frame = std::move(m_queue.front());
    m_queue.pop_front();
    return frame;
}
```

- 检查、move、pop 在同一锁内完成。
- `std::move(front)` 把帧所有权转给消费者；deque 中留下空 unique_ptr，随后 pop 删除容器节点。
- 返回后消费者独占 frame，可以在不持有队列锁的情况下 memcpy，避免大帧复制阻塞生产者。

这是队列设计中正确且重要的一点：锁只保护容器元数据，不覆盖昂贵的数据处理。

### 42.4 `empty()` 后再 `pop()` 的竞争

`rtspFrameCall()` 等待 I 帧时写成：

```cpp
while (!videoQueue->empty())
{
    auto frame = videoQueue->pop();
```

`empty()` 和 `pop()` 分别加锁，不是一个原子操作。若有多个消费者，二者之间另一个消费者可能拿走最后一帧，使 pop 返回 null。

当前每个共享 source 通常只有 live555 event loop 一个消费者，所以风险被架构约束压低，但队列 API 本身不能保证这段组合操作原子。

### 42.5 `pop_wait()` 为什么当前没有发挥作用

`pop_wait()` 使用 condition variable，可无限或超时等待：

```cpp
m_cv.wait_for(lock, timeout,
    [this]() { return !m_queue.empty() || m_bStop; });
```

谓词防止虚假唤醒；`m_bStop` 允许退出时唤醒等待线程。

但 live555 source 不能简单地在 event loop 线程无限阻塞，否则整个 RTSP 服务不能处理 socket 和其他客户端。因此当前 source 选择 `scheduleDelayedTask(10ms)` 轮询。若要用 condition variable，应让独立桥接线程等待，再用 live555 event trigger 通知 event loop，而不是直接在 event loop 阻塞。

### 42.6 `clear()`

```cpp
void clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.clear();
}
```

deque clear 会析构每个 `unique_ptr<FrameData>`，进一步析构其中 `unique_ptr<unsigned char[]>`，因此队列内帧内存可自动回收。

但这不等于队列对象本身被销毁。`Live_Stream_Info_t` 用 free 释放时不会调用其 queue unique_ptr 析构，仍存在队列对象泄漏/未定义行为。

## 43. `CRtspServer` 成员与构造函数：配置如何进入运行状态

源码位置：`rtsp_server.h:64-313`、`rtsp_server.cpp:165-200`。

### 43.1 `Live_Stream_Info_t` 是每路流的共享状态

关键字段：

```cpp
char streamName[STREAM_NAME_MAX];
std::unique_ptr<CThreadSafeFrameQueue> videoQueue;
std::unique_ptr<CThreadSafeFrameQueue> audioQueue;
volatile int request;
volatile int requestIFrame;
char achUrl[64];
float fFps;
```

- `streamName` 是 live555 session key，不是展示名称。
- 两个 queue 是 VENC/AENC 生产者与 live555 消费者的桥。
- `request` 表示至少有一条路径要求上层生产 RTSP 数据。
- `requestIFrame` 表示消费者正在等待随机访问序列起点。
- `achUrl` 是不带认证的 URL。
- `fFps` 传给 video source，用于推导 duration。

`volatile` 注释写“确保多线程可见性”并不准确。C++ 中 volatile 不提供原子性、happens-before 或防止数据竞争；它主要用于特殊内存访问语义和禁止部分优化。

### 43.2 构造函数读取哪些配置

```cpp
Convert::read_file(VIDEO_CONFIG_FILE, m_vstVideoConfig);
Convert::read_file(AUDIO_CONFIG_FILE, m_stAudioConfig);
```

RTSP session 的 codec、fps、是否带音频在 init 时从这两个成员快照生成。运行中修改配置时必须先调用 `setVideoConfig/setAudioConfig` 更新快照，再 reboot 重建 subsession。

```cpp
Network::PortConfig_S stPortConfig;
if (Convert::read_file(m_strPortPath, stPortConfig))
{
    Convert::write_file(m_strPortPath, stPortConfig);
}
m_nRtspPort = stPortConfig.nRtspPort;
```

- 文件不存在时写默认端口配置；
- 成员默认是 554，但最终以配置结构为准；
- 低于 1024 的端口在 Linux 上通常需要相应权限，设备一般以高权限进程运行。

```cpp
m_nMediaDscp = QOS_DSCP_MIN;
CQosManage::instance()->get_qos_config(m_stQosConfigInfo);
if (validRange)
{
    m_nMediaDscp = configuredValue;
}
```

先设置安全默认值，再只接受范围合法的配置，避免无效 DSCP 传入 socket option。

```cpp
m_bAuthentication = true;
updateRtspDigestAlgorithm();
```

- 当前实现强制开启 RTSP 认证；
- 用户名/密码来自类成员默认值或后续 `update_userInfo()`；
- Digest 算法从安全配置读取。

构造函数只读取配置，不创建 socket 或线程。真正外部副作用集中在 `init()`。

## 44. `CRtspServer::init()`：两路 RTSP session 是怎样逐项创建的

源码位置：`rtsp_server.cpp:206-370`。

### 44.1 分配总上下文

```cpp
m_pLiveInfo = (LIVE_RTSP_S*)calloc(1, sizeof(LIVE_RTSP_S));
if (m_pLiveInfo == NULL)
{
    return ERR_PARAM_NULL;
}
```

- `calloc` 把端口、handle 和两个 `listLive` 指针清零，便于失败路径判断哪些资源已创建。
- `LIVE_RTSP_S` 目前只有 C 风格标量和指针，用 calloc 本身没有非平凡构造问题。
- 如果类允许重复调用 init，而旧 `m_pLiveInfo` 未释放，这里会覆盖旧指针。外层依赖 `isInit()` 防止重复 init。

### 44.2 创建 live555 服务器句柄

```cpp
m_pLiveInfo->pServerHandle = rtsp_server_init(
    m_nRtspPort,
    m_bAuthentication,
    m_strUser.c_str(),
    m_strPwd.c_str(),
    m_nRtspDigestAlgorithm,
    m_nMediaDscp);
```

参数逐一映射：

- `m_nRtspPort`：RTSP TCP 监听端口；
- `m_bAuthentication`：是否创建 `UserAuthenticationDatabase`；
- user/password：加入认证数据库；
- Digest algorithm：MD5、SHA-256 或组合模式；
- DSCP：用于监听/媒体 socket 的 QoS 标记。

返回类型是 `void*` 句柄，业务层不知道内部 `TaskScheduler`、`RTSPServer` 等 C++ 类型，实现了 ABI 隔离。

失败分支：

```cpp
if (pServerHandle == NULL)
{
    return ERR_PARAM_NULL;
}
```

这里直接返回，没有 free 刚分配的 `m_pLiveInfo`。因此端口占用等 init 失败会泄漏总上下文，并给下一次 init 留下状态复杂性。

### 44.3 获取 IP 只用于生成 URL

```cpp
Network::Info_S stNetInfo;
CNetworkManage::instance()->get_system_networkInfo(stNetInfo);
```

随后 IP 被写入 `achUrl`。live555 的监听地址并不是靠这个字符串 bind；服务器通常在通配地址监听。因此把这里的 IP 改成 127.0.0.1 不会自然地把 server 限制为 loopback。

```cpp
set_handshakeAuth_callback(serverHandle, handshakeAuth_callback);
```

该回调当前只校验参数并记录客户端 IP，没有执行额外授权。真正的用户名密码校验在 live555 `authenticationOK()`。

### 44.4 为每路创建 `Live_Stream_Info_t`

```cpp
for (int i = 0; i < RTSP_CHN_MAX; i++)
{
    m_pLiveInfo->listLive[i] =
        (Live_Stream_Info_t*)calloc(1, sizeof(Live_Stream_Info_t));
```

`i=0` 是主码流，`i=1` 是子码流。

这里有严重 C++ 生命周期问题：`Live_Stream_Info_t` 含两个 `std::unique_ptr`，不能用 calloc 跳过构造函数。后续给未构造 unique_ptr 赋值属于未定义行为。正确设计应使用 `new Live_Stream_Info_t{}` 或 `std::make_unique`。

```cpp
listLive[i]->requestIFrame = 0;
listLive[i]->fFps = m_vstVideoConfig[i].getFrameRateAsFloat();
memset(&m_stClientInfo[i], 0, sizeof(Rtsp_Create_Info_t));
```

- 初始不等待 I 帧，因为还没有客户端；
- fps 固化到每路共享状态，video source 每次取帧通过 callback 获得；
- `Rtsp_Create_Info_t` 是纯 C 接口结构，memset 清零确保未设置的 Audioindex 等为 null。

### 44.5 URL 构造

```cpp
snprintf(listLive[i]->achUrl,
         sizeof(listLive[i]->achUrl),
         RTSP_URL_DEFAULT,
         ip, port, i + 101);
```

- `i+101` 形成行业常见的 101/102 资源编号；
- 使用 snprintf 防止写越界；
- `achUrl[64]` 对 IPv4 通常够用，对较长 IPv6 文本或将来更长路径可能截断；
- 返回值没有检查，因此发生截断时仍把不完整 URL 暴露给上层。

认证 URL 使用 `std::string` 先 resize 128，再 snprintf 到 `data()`：

```cpp
auto& Url = m_rtspUrlMap[i];
Url.resize(128);
snprintf(Url.data(), Url.size(),
         "rtsp://%s:%s@%s:%d/...", ...);
```

这种写法能获得可写连续缓冲，但 string 的逻辑 size 保持 128，末尾包含大量 `\0`。作为 C 字符串返回时能工作，作为普通 string 比较、序列化或长度统计时容易产生意外。更稳妥的是先计算返回长度再 resize 到实际字符数。

### 44.6 根据视频配置选择 subsession 类型

```cpp
if (codec == H264)
    nProtolType = RTSP_FRAMEPROTOL_H264;
else if (codec == H265)
    nProtolType = RTSP_FRAMEPROTOL_H265;
else if (codec == MJPEG)
    nProtolType = RTSP_FRAMEPROTOL_MJPEG;
```

这个值不是 IP 协议类型，而是 mediaServer 封装层选择 H264/H265/MJPEG subsession 的枚举。

没有最终 `else`：若 codec 是 SVAC3/JPEG/未知值，结构体因 memset 保持 0。若 0 恰好对应 H.264，就可能错误创建 H.264 session。这就是为什么所有不支持 codec 都应该显式拒绝，而不是依赖零值。

### 44.7 队列创建与回调上下文

```cpp
videoQueue = std::make_unique<CThreadSafeFrameQueue>(MAX_VIDEO_FRAME);
audioQueue = std::make_unique<CThreadSafeFrameQueue>(MAX_AUDIO_FRAME);

sprintf(streamName, "Streaming/Channels/%d", i + 101);
m_stClientInfo[i].Videoindex = listLive[i];
m_stClientInfo[i].clientFun = rtspStateCallback;
m_stClientInfo[i].dataGetfun = rtspFrameCall;
```

- queue 属于每路 session，不是每个客户端；
- `Videoindex` 被当作 `void*` 透传，在 callback 中 cast 回 `Live_Stream_Info_t*`；
- `clientFun` 是控制方向：live555 → IPC 状态；
- `dataGetfun` 是数据方向：live555 → IPC 取帧。

这是整个 RTSP 适配的关键反转：VENC 负责 push 进队列，但 live555 不是由 push 直接驱动，它按自己的 event loop 节奏调用 `dataGetfun` pull。

### 44.8 是否创建音频 track

```cpp
if (enVideoType == COMPOSITE_STREAM)
    Audioindex = listLive[i];
else if (enVideoType == VIDEO_STREAM)
    Audioindex = NULL;
```

- `Audioindex != null` 是 mediaServer 层添加 audio subsession 的开关；
- 它不是“音频通道号”，而是音频 source callback 的上下文；
- 纯视频模式下，后续 `sendAudioData()` 也会因 Audioindex null 直接返回。

### 44.9 音频枚举映射

```cpp
G711U -> nAudioType = 1;
G711A -> nAudioType = 2;
G726  -> nAudioType = 3, bitWidth = 4;
AAC   -> nAudioType = 0;
```

这个数值协议是 IPC 与预编译 `libRtspServer.so` 的 ABI 契约。修改枚举顺序不能只改一边，否则会把 G.711U 当成 G.711A 等。

AAC 还映射 sampling frequency index，因为 SDP AudioSpecificConfig 用索引而不是直接采样率。无法支持的采样率会把 `Audioindex` 置 null，使该 session 退化为纯视频。

### 44.10 创建 session 时返回值被忽略

```cpp
m_stClientInfo[i].param1 = MAX_CLIENT_NUM;
strcpy(m_stClientInfo[i].streamName, streamName);
rtsp_server_create(serverHandle, &m_stClientInfo[i]);
```

- param1 试图表达每路最大客户端数 4；
- streamName 是 live555 哈希查找 key；
- `rtsp_server_create` 失败返回 -1，但这里不检查。

即使主路或子路 session 创建失败，循环结束后仍执行：

```cpp
m_bInitFlag.store(true);
return OK;
```

因此 `isInit()==true` 只证明全局 init 流程走到结尾，不证明两路 URL 都能 lookup 成功。

## 45. `sendVideoData()/sendAudioData()`：生产线程如何安全跨越 MPP 生命周期

### 45.1 init flag 只能做第一层门控

```cpp
if (!m_bInitFlag.load())
{
    return ERR;
}
Live_Stream_Info_t* pStreamInfo = m_pLiveInfo->listLive[nChannel];
```

- atomic load 防止明显的未初始化访问；
- 但“检查为 true”与下一行解引用之间，控制线程可能开始 deinit 并释放 `m_pLiveInfo`；
- 因此它不能单独解决 check-then-use 竞争。需要共享锁、引用计数或停止生产线程后再释放。

还缺少 `nChannel` 范围和 `pVideoFrame` 空指针检查，当前依赖上层调用契约。

### 45.2 为什么只有 `request==1` 才复制

```cpp
if (pStreamInfo->request == 1)
{
    auto frameData = std::make_unique<FrameData>();
    frameData->data =
        std::make_unique<unsigned char[]>(pVideoFrame->nLen);
```

- 没客户端时不为 RTSP 分配每帧内存，降低常驻开销；
- `request` 由 live555 source/PLAY 状态回调设置；
- `FrameData` 和其字节数组是两次 heap allocation，30 fps × 多个 pack × 两路时分配频率较高；
- 可以考虑单块 frame 对象、对象池或 slab，但先要用性能数据证明 allocator 是瓶颈。

### 45.3 第二次深拷贝为什么必要

```cpp
memcpy(frameData->data.get(),
       pVideoFrame->pData,
       pVideoFrame->nLen);
```

`pVideoFrame` 在 VENC 当前循环末尾就释放，而 queue 要跨线程保存到 live555 未来某个调度时刻，所以必须建立独立所有权。

如果想减少这次复制，应重新设计 `VideoFrame_S` 本身的共享所有权，让 RTSP queue 接管或共享同一块 buffer，而不能简单保存裸指针。

### 45.4 起播标志为什么标 SPS/VPS

```cpp
if (codec == H264)
    iFrame = (eType == H264_TYPE_SPS);
else if (codec == H265)
    iFrame = (eType == H265_TYPE_VPS);
else if (codec == MJPEG)
    iFrame = 1;
```

新客户端需要的不是孤立 IDR slice，而是：

```text
参数集 -> 随机访问图像
```

所以 H.264 从 SPS 开始，H.265 从 VPS 开始。字段命名应理解为 `randomAccessSequenceStart`。

隐含前提是 VENC 每次请求 IDR 时都会依次重新输出参数集。如果编码器只输出 IDR、不重复 VPS/SPS/PPS，`requestIFrame` 会一直等待。

### 45.5 队列满时发生什么

```cpp
if (!videoQueue->push(std::move(frameData)))
{
    dlog_debug("视频队列已满，丢弃帧");
}
```

- move 后调用方不再拥有 frame；
- push 满返回 false 时由形参析构释放；
- 丢的是最新 pack，不是最旧 pack；
- 如果最新 pack 是 VPS/SPS/IDR，起播会继续等待下一次 GOP；
- debug 级逐帧日志在持续拥塞时可能产生大量日志开销。

### 45.6 无请求时为什么主动 `clear()`

```cpp
else
{
    videoQueue->clear();
}
```

客户端停止的状态回调与 VENC 生产线程之间存在时间窗口。clear 保证即使队列里残留旧 GOP，下次客户端连接也不会先播放陈旧 P 帧，而会重新请求 IDR。

### 45.7 `CRtspServer::sendAudioData()`：音频路径的不同点

```cpp
if (!m_bInitFlag.load() ||
    m_stClientInfo[nChannel].Audioindex == nullptr)
{
    return ERR;
}
```

音频先检查该 session 是否配置 audio track。否则没有消费者，入队只会浪费内存。

音频 frame 没有关键帧标记，其他流程与视频相同：深拷贝、入队、满队列丢新、无请求清队列。

## 46. `rtspStateCallback()`：PLAY、DESCRIBE 与 IDR 状态机

源码位置：`rtsp_server.cpp:108-145`。

### 46.1 参数检查与上下文恢复

```cpp
if (param == nullptr || param->param == nullptr)
{
    return ERR;
}
Live_Stream_Info_t* pStreamInfo =
    static_cast<Live_Stream_Info_t*>(param->param);
```

- 第一层 param 是 live555 传来的状态结构；
- 第二层 `param->param` 是 init 时写入 `Videoindex` 的上下文；
- static_cast 从无类型 ABI 恢复成业务对象。

### 46.2 STOP

```cpp
if (param->status == RTSPCLIENT_STOP)
{
    pStreamInfo->request = 0;
    pStreamInfo->requestIFrame = 0;
}
```

- request=0 使 VENC/AENC 生产线程停止为 RTSP 分配 frame；
- requestIFrame 清零，避免下次连接继承旧等待状态；
- 实际队列不是在这里 clear，而是在下一次 `sendVideoData/sendAudioData` 看到 request=0 时清理。

如果编码器因某种原因此后不再产生帧，旧队列可能一直保留到 deinit。状态回调中直接 clear 会更确定，但需要评估与 `rtspFrameCall` 并发。

### 46.3 START

```cpp
pStreamInfo->request = 1;
pStreamInfo->requestIFrame = 1;
```

- 先打开生产门，再进入等待参数集模式；
- 如果顺序反过来，消费者可能先看到 requestIFrame=1，但生产者仍不入队；
- 两个字段不是 atomic，所以源码层面仍无正式的跨线程顺序保证。

### 46.4 从指针反查通道号

```cpp
int nChannel = 0;
for (int i = 0; i < RTSP_CHN_MAX; i++)
{
    if (m_pLiveInfo && m_pLiveInfo->listLive[i] == pStreamInfo)
    {
        nChannel = i;
        break;
    }
}
```

- callback ABI 没有直接传 channel，只传上下文指针，所以需要反查；
- 默认 0 意味着如果没有匹配到，会错误请求主码流 IDR；
- 更安全的结构是在 `Live_Stream_Info_t` 内保存明确 channel id，未匹配时返回错误。

### 46.5 触发 IDR

```cpp
CRtspServer::instance()->triggerRequestIdr(nChannel);
```

为什么状态回调不直接调用 MPP：RTSP 模块不应该依赖海思 VENC 具体类型。它只通过注册的回调访问视频模块，保持模块边界。

### 46.6 START 不只来自 PLAY

自定义 H.264/H.265 source 的 `set_videostate_callback()` 在第一次取帧时也发 START。DESCRIBE 为生成 SDP 会启动 dummy source，因此可能出现：

```text
DESCRIBE -> dummy source START -> 请求 IDR
SDP 参数集取得 -> dummy source 析构 -> STOP
PLAY -> real source/startStream START -> 再请求 IDR
```

这解释了为什么一次客户端连接可能看到多次 START/STOP/IDR。当前用单 bool request，而不是按 source/client 引用计数，复杂并发连接下可能被一个 dummy STOP 提前清零。

## 47. `rtspFrameCall()`：live555 每次取帧究竟做了什么

源码位置：`rtsp_server.cpp:18-103`。

### 47.1 为什么目标地址由 live555 提供

```cpp
if (frame == nullptr || frame->param == nullptr || frame->data == nullptr)
    return ERR_PARAM_NULL;
```

`frame->data` 指向 `FramedSource::fTo`，由 live555 根据 `OutPacketBuffer`/source max size 准备。业务 callback 的职责是把一项数据复制进去，并填写实际 `frameSize`。

接口没有 `capacity` 字段，所以 callback 无法验证目标缓冲是否容纳 `FrameData::frameSize`，这是 ABI 设计缺口。

### 47.2 音频取帧

```cpp
auto audioFrame = pStreamInfo->audioQueue->pop();
if (audioFrame)
{
    memcpy(frame->data,
           audioFrame->data.get(),
           audioFrame->frameSize);
    frame->frameSize = audioFrame->frameSize;
}
else
{
    frame->frameSize = 0;
}
```

- pop 把 unique_ptr 所有权转给当前 callback；
- memcpy 在不持有 queue mutex 的情况下执行；
- callback 返回后局部 unique_ptr 析构，释放 FrameData；
- 空队列用 size=0 告诉 source 暂时无数据，source 会延迟后重试。

### 47.3 等待随机访问起点

```cpp
if (pStreamInfo->requestIFrame == 1)
{
    frame->frameSize = 0;
    frame->iFrame = 0;

    while (!videoQueue->empty())
    {
        auto videoFrame = videoQueue->pop();
```

进入该模式后，本次 callback 可能连续丢弃多个 pack。它不是“只看队首一次”，是主动清掉从旧 GOP 遗留的非起点数据。

```cpp
if (videoFrame->iFrame == 1)
{
    memcpy(...);
    frame->frameSize = videoFrame->frameSize;
    frame->iFrame = 1;
    pStreamInfo->requestIFrame = 0;
    break;
}
```

- 找到 SPS/VPS 后立即把这一项交给 live555；
- 清除等待标志，使下一次 callback 正常取 PPS、IDR 等后续项；
- 只要队列顺序正确，就能把参数集序列完整送出。

如果队列是：

```text
SPS -> P slice -> PPS -> IDR
```

等待标志在 SPS 后就清除，错误的 P slice 也会送给客户端。该算法依赖编码器输出的 pack 顺序符合标准随机访问序列。

### 47.4 普通模式

```cpp
auto videoFrame = videoQueue->pop();
if (videoFrame)
{
    memcpy(...);
    frame->frameSize = videoFrame->frameSize;
    frame->iFrame = videoFrame->iFrame;
}
```

每次 callback 只交付一个 queue item。后续 `H264VideoStreamFramer/H265VideoStreamFramer` 负责把输入字节流解析成 NAL 和 Access Unit。

### 47.5 为什么回传队列深度与 fps

```cpp
frame->videolistsize = videoQueue->size();
frame->audiolistsize = audioQueue->size();
frame->fFps = pStreamInfo->fFps;
```

- source 用 queue size 判断是否积压；深度 >=2 时把 duration 置 0，加速下一次读取；
- fps 用来计算单帧理论 duration；
- size() 每次单独加锁，值是一个瞬时快照，返回后生产者可能立刻改变队列。

`frame->fFps` 没有从实际 VENC PTS 测量，而是配置值。如果编码器降帧或 sensor 实际 fps 变化，RTP pacing 会与真实采集节奏偏离。

## 48. `triggerRequestIdr()`：跨模块回调、频控与锁

源码位置：`rtsp_server.cpp:634-678`。

### 48.1 MJPEG 为什么直接返回

```cpp
if (m_vstVideoConfig[nChannel].enVideoCodec == MJPEG)
{
    return OK;
}
```

MJPEG 每张 JPEG 独立可解码，没有参考帧和 IDR 概念。请求 VENC IDR 对 MJPEG 无意义，也可能让 MPP 返回不支持。

### 48.2 `m_mutexCtrl` 保护什么

```cpp
std::lock_guard<std::mutex> lock(m_mutexCtrl);
```

同一把锁也被 `CRtspServer::reboot()` 持有。这里的意图是避免：

- RTSP 正在 deinit/reinit 时触发 IDR；
- 多个 START 同时更新 `m_lastIdrRequestTimeMap`；
- callback 成员在配置更新时被并发替换。

但把外部 callback 调用放在锁内会扩大临界区。如果 callback 将来反向调用需要同一 mutex 的 RTSP 方法，可能形成重入死锁。

### 48.3 时间频控

```cpp
auto now = TimeUtils_NS::get_currentTimestampMs();
auto& lastTime = m_lastIdrRequestTimeMap[nChannel];
auto elapsedMs = now - lastTime;
if (elapsedMs < m_minIdrInterval)
{
    return OK;
}
```

- `operator[]` 在通道第一次出现时创建默认值；若值为 0，第一次 elapsed 很大，会允许请求。
- 30 ms 不是 GOP 间隔，而是去抖时间，用于合并 DESCRIBE/PLAY 等近同时请求。
- 被频控也返回 OK，语义是“请求已被策略接受但无需再次执行”，调用方不会当成错误。
- 若系统时间回拨，`now-lastTime` 可能为负并长时间拒绝请求。频控应优先使用单调时钟。

### 48.4 视频初始化门控

```cpp
if (m_requestIdrCallback)
{
    if (*static_cast<bool*>(m_pCallbackUserData) == true)
    {
        m_requestIdrCallback(nChannel, m_pCallbackUserData);
        lastTime = now;
    }
}
```

- 先检查 `std::function` 非空，避免调用空函数对象抛出异常；
- userData 被强制解释为 `bool*`，这使通用 void* 接口实际上绑定到特定类型；
- 没有检查 `m_pCallbackUserData` 本身非空；
- callback 执行后才更新时间，失败返回无法表达，因为 callback 类型是 void；
- 即使底层 `ss_mpi_venc_request_idr` 失败，lastTime 仍更新，30 ms 内不会重试。

## 49. `CRtspServer::deinit()/reboot()`：资源释放的真实顺序

### 49.1 先关闭生产门

```cpp
m_bInitFlag.store(false);
```

它使之后进入 `sendVideoData/sendAudioData` 的调用立即返回。但已经通过 load 检查、正准备解引用 `m_pLiveInfo` 的生产线程不受保护，因此仍需要与生产线程建立更强同步。

### 49.2 清队列、销毁 session、释放上下文

```cpp
for (int i = 0; i < RTSP_CHN_MAX; i++)
{
    videoQueue->clear();
    audioQueue->clear();
    rtsp_server_destory(serverHandle, streamName);
    free(listLive[i]);
}
```

正确意图是：

```text
不再入队
 -> 释放排队帧
 -> 让 event loop 删除 ServerMediaSession
 -> 释放 IPC 每路上下文
```

风险逐项看：

- 没有检查 `m_pLiveInfo`、`listLive[i]`，只适用于完整成功的 init；
- `rtsp_server_destory` 拼写虽错但属于既有 ABI，不能随意只改头文件；
- destroy 返回值未检查，即使 event loop 删除失败也继续 free callback 上下文；
- live555 source 若仍持有 `Videoindex=listLive[i]`，随后 callback 会访问已释放地址；
- `free` 不调用其中 unique_ptr 析构，queue 对象本身不会正常 delete。

### 49.3 全局 live555 uninit

```cpp
rtsp_server_unInit(m_pLiveInfo->pServerHandle);
free(m_pLiveInfo);
m_pLiveInfo = nullptr;
```

理论上先销毁所有 session，再停止 event loop 和 RTSP server，最后释放业务总上下文。

实际 `rtsp_server_unInit()` 没有 join event loop 线程，却会删除 scheduler/environment。即使单路 session destroy 使用 event trigger 同步成功，全局 event thread 仍可能在下一次 `doEvent()` 访问已释放对象。

### 49.4 `reboot()`

```cpp
std::lock_guard<std::mutex> lock(m_mutexCtrl);
if (m_bInitFlag.load())
{
    deinit();
}
init();
```

- mutex 串行化多个配置线程的 reboot；
- deinit 后立即 init，旧客户端全部断开；
- 新 session 会重新读取成员中的 codec/audio/port/digest 快照；
- 即使调用前 RTSP 是 disabled，只要 `m_bInitFlag=false`，函数仍直接 init，这会意外重新开启服务。

`setPort()`、`setQosDscp()` 都无条件 reboot，所以它们应先检查 RTSP enable 状态，或者 reboot 应记住原始运行状态。

## 50. IPC 层源码执行时序：把前述函数连成一条真实时间线

### 50.1 服务启动时

```text
主线程
  CStreamVideo::init
    创建 handler/parser
    注册 IDR callback
    SYS/VB -> VI/ISP -> VPSS -> VENC
    启动 VENC threads
    绑定 VPSS->VENC

  CStreamAudio::init
    启动 AENC 相关线程

  CPushStream::init
    CRtspServer::init
      rtsp_server_init -> live555 event thread
      创建 /101 和 /102 上下文与队列
      rtsp_server_create -> ServerMediaSession
```

VENC 线程此时一直取编码 pack，但在 `request==0` 时 RTSP 不做深拷贝，只持续清空队列。

### 50.2 客户端连接时

```text
live555 event thread
  DESCRIBE
    dummy source 首次 getNextFrame
    rtspStateCallback START
      request=1
      requestIFrame=1
      triggerRequestIdr

VENC thread
  收到编码器新 VPS/SPS/PPS/IDR pack
  CRtspServer::sendVideoData
  queue.push

live555 event thread
  rtspFrameCall
  丢弃旧数据直到 VPS/SPS
  复制到 fTo
  framer 解析参数集
  生成 SDP
```

### 50.3 PLAY 后持续发送

```text
live555 PLAY -> startStream -> START callback
VENC thread -> queue.push
event loop -> source delayed task -> rtspFrameCall -> queue.pop
framer -> RTP sink -> UDP/TCP
RTCP scheduler -> 周期 SR
```

### 50.4 最后一个客户端退出

```text
TEARDOWN/socket timeout
  -> deleteStream
  -> RTSPCLIENT_STOP
  -> request=0
  -> 后续 VENC/AENC send 调用 clear queue
```

这一时序是后续理解 live555 内部函数的基础：IPC 生产者线程从不直接调用 RTP send；live555 event loop 也不直接访问 MPP pack，它们只通过队列和两个 callback 交汇。

## 51. `rtsp_server_init()`：C 接口背后创建了哪些 live555 对象

源码位置：`mediaServer/rtspServer_base.cpp:152-282`。

### 51.1 为什么返回 `void*`

```cpp
RtSpServerHandle_t rtsp_server_init(...)
```

`RtSpServerHandle_t` 实际是 `void*`。它让 IPC 主程序只依赖一个稳定的 C 头文件，不需要暴露 live555 的类布局、STL 类型和编译器 ABI。

函数内部把它解释为：

```cpp
Rtsp_Server_Info_t* pServeInfo =
    (Rtsp_Server_Info_t*)malloc(sizeof(Rtsp_Server_Info_t));
memset(pServeInfo, 0, sizeof(Rtsp_Server_Info_t));
```

该结构当前包含 `std::atomic<char> m_aQuit{0}`。与上层 `Live_Stream_Info_t` 类似，用 malloc/memset 创建含 C++ 非平凡成员的对象会绕过构造函数，严格来说仍是未定义行为。`std::atomic` 不能被当作普通 C POD 假设。

### 51.2 `OutPacketBuffer::maxSize`

```cpp
OutPacketBuffer::maxSize = REV_BUF_SIZE;
```

- 全局静态上限被设置为 2.5 MiB；
- 必须在创建 RTP sink 之前设置，因为 sink/fragmenter 构造时会据此分配缓冲；
- 它用于容纳一个较大的输入 NAL/JPEG/AAC frame，不是 UDP MTU；
- 这是进程全局值，如果同进程还有其他 live555 server/source，也会受影响。

### 51.3 scheduler 与 environment

```cpp
pServeInfo->scheduler = BasicTaskScheduler::createNew();
pServeInfo->usage_env =
    BasicUsageEnvironment::createNew(*pServeInfo->scheduler);
```

- scheduler 管理 fd 可读事件、延迟任务和 event trigger；
- environment 包装 scheduler，同时保存错误消息和输出接口；
- environment 引用 scheduler，所以销毁顺序必须 environment 在前、scheduler 在后。

### 51.4 创建认证数据库

```cpp
if (nRegister && pUser && pPassworld)
{
    char const* realm = "Itc Streaming Server";
    authDB = new UserAuthenticationDatabase(realm, False);
    authDB->addUserRecord(pUser, pPassworld);
}
```

- 三个条件同时满足才启用；空用户名指针会退化为无鉴权；
- `False` 表示数据库里的 password 不是预计算 MD5 HA1，而是明文；
- realm 参与 Digest HA1，客户端计算时必须与 challenge 完全一致；
- RTSPServer 头文件注明 authDatabase 由调用者负责回收，但当前 wrapper 没有保存独立 authDB 指针并 delete，存在泄漏。

### 51.5 创建监听 socket

```cpp
pServeInfo->rtspServer = RTSPServer::createNew(
    *usage_env, nDscp, port, authDB, 65, algorithm);
```

参数 65 是 client session 回收超时秒数：65 秒内没有 RTSP 命令或 RTCP RR，live555 可以回收该客户端 session。

`RTSPServer::createNew()` 同时尝试 IPv4 和 IPv6：

```cpp
ourSocketIPv4 = setUpOurSocket(..., AF_INET, dscp);
ourSocketIPv6 = setUpOurSocket(..., AF_INET6, dscp);
if (both < 0) return NULL;
```

只要 IPv4/IPv6 任意一个成功就创建 server。因此 IPv6 bind 失败不必然导致服务失败。

### 51.6 control event trigger

```cpp
controlTriggerId = scheduler->createEventTrigger(
    handleRtspControlEvent);
```

live555 对象应由 event loop 线程操作。控制线程调用 destroy 时不能直接 delete session，于是通过 trigger 把一个任务投递到 event loop。

EventTrigger 与 frame queue 的 condition variable 不同：它用于跨线程唤醒 live555 scheduler，并在正确线程执行 callback。

### 51.7 pthread 属性配置为何没有生效

```cpp
pthread_attr_init(&eventThattr);
pthread_attr_setdetachstate(&eventThattr, PTHREAD_CREATE_DETACHED);
pthread_create(&eventLoopTid, NULL, doEvenLoopThread, pServeInfo);
```

前两行把 `eventThattr` 配成 detached，但 `pthread_create` 第 2 参数传的是 `NULL`，不是 `&eventThattr`。因此实际线程使用默认 joinable 属性。

随后 uninit 又注释掉 join eventLoopTid，形成：

- 线程实际可 join；
- 代码没有 join；
- scheduler/environment 先被释放。

这是明确的生命周期缺陷。

### 51.8 为什么还有 error print 线程

```cpp
pthread_create(&printErrorTid, NULL, doPrintError, pServeInfo);
```

live555 很多错误只写入 `UsageEnvironment::setResultMsg()`。打印线程每秒读取并清空，使后台错误进入项目日志。

但 environment 的 result message 若没有内部锁，event loop 写、print thread 读可能发生数据竞争。更自然的设计是在 event loop 内记录错误或使用线程安全日志桥。

## 52. `doEvenLoopThread()`：为什么项目没有直接用 `doEventLoop()`

源码位置：`rtspServer_base.cpp:678-700`。

```cpp
while (true)
{
    if (m_aQuit.load(std::memory_order_seq_cst) != 0)
        break;

    usage_env->taskScheduler().doEvent();
}
```

标准 live555 常写：

```cpp
scheduler->doEventLoop(&watchVariable);
```

项目改为每次只执行一个 `doEvent()`，每个事件返回后检查 atomic quit。这样退出条件更直观，也能插入自定义控制。

`seq_cst` 使用最强内存序，确保退出标志在所有线程看到统一顺序。对于单一退出布尔，acquire/release 通常已经足够，但 seq_cst 更容易推理。

关键问题在于 `doEvent()` 可能等待下一个 socket/timer 事件。设置 quit 本身未触发 scheduler wakeup，退出延迟取决于 scheduler 最近任务；若随后不 join，调用者根本不知道线程何时真正停止。

## 53. `rtsp_server_create()`：从一个 C 结构变成视频、音频 track

源码位置：`rtspServer_base.cpp:284-467`。

### 53.1 复制 callback 配置

```cpp
Audio_Source_Info_t audioInfo{};
Video_Source_Info_t videoInfo{};

audioInfo.clientFun = createInfo->clientFun;
audioInfo.dataGetfun = createInfo->dataGetfun;
audioInfo.audioindex = createInfo->Audioindex;

videoInfo.clientFun = createInfo->clientFun;
videoInfo.dataGetfun = createInfo->dataGetfun;
videoInfo.videoindex = createInfo->Videoindex;
```

- video/audio source 共用同一个 `rtspFrameCall`，通过 `Fream_Info_t::type` 区分；
- client state 主要由 video source/subsession 发出；
- index 指针分别作为 callback 上下文；当前两者都指向同一个 `Live_Stream_Info_t`。

结构体按值复制到 subsession，确保 IPC 侧 `m_stClientInfo` 后续变化不会破坏已创建 source 的函数指针。

### 53.2 客户端上限设置顺序错误

```cpp
rtsp_setclient_maxNum(handle, streamName, param1);
```

此时 `ServerMediaSession` 尚未 `createNew` 并加入数组，`findstreamName()` 必然找不到首次创建的 stream，函数返回 -1。返回值未检查，所以配置静默失效。

正确顺序应是 session create/add 后设置，且准入逻辑必须真正读取 `fReferenceMax`。

### 53.3 H.264 分支

```cpp
server_session[nUse] = ServerMediaSession::createNew(
    *env, streamName, 0, "Session from MainStream");
```

参数含义：

- streamName：URL suffix key；
- info 传 0/null；
- description 会出现在 SDP 的 `s=` 等字段。

```cpp
server_session[nUse]->addSubsession(
    H264_Server_Subsession::createNew(*env, videoInfo));
```

这创建视频 track。若 Audioindex 非空，根据 `nAudioType` 再添加 AAC/G711/G726 track。

最后：

```cpp
rtspServer->addServerMediaSession(server_session[nUse]);
```

`GenericMediaServer` 把 session 放进哈希表，之后 DESCRIBE/SETUP 才能用 streamName lookup。

H.265、MJPEG 分支结构相同，只替换 video subsession 类。

### 53.4 `nUse` 与数组容量

`findServerMediaSessionUse()` 查找 8 项数组中首个 null，并递增 `nUse`。它没有 mutex；当前 create 通常只在控制线程串行执行，依赖外层生命周期锁。

如果多个线程同时创建 session，可能选中同一空槽。

### 53.5 SDP URL 打印

```cpp
char* url = rtspServer->rtspURL(session);
*usage_env << "using url: \"" << url << "\"\n";
delete[] url;
```

- `rtspURL()` 返回 new[] 字符串，调用者负责 delete[]；
- 该 URL 由 live555 根据本机地址和 session name 生成，不带用户名密码；
- 它用于诊断，不是 session 自身保存的绑定地址。

## 54. `rtsp_server_destory()`：跨线程删除 session 为什么要等待

源码位置：`rtspServer_base.cpp:469-503`。

### 54.1 event loop 内调用可直接删除

```cpp
if (pthread_equal(pthread_self(), eventLoopTid) ||
    controlTriggerId == 0)
{
    return cleanServerMediaSession(...);
}
```

- 如果当前就是 event loop，再 trigger 并等待自己会死锁，所以直接执行；
- trigger 不可用时也只能直接执行，但从其他线程直接操作 live555 对象存在风险，这只是降级路径。

### 54.2 栈上控制任务

```cpp
Rtsp_Control_Task_t task;
memset(&task, 0, sizeof(task));
task.pServerInfo = pServerInfo;
snprintf(task.streamName, ...);
pthread_mutex_init(&task.mutex, NULL);
pthread_cond_init(&task.cond, NULL);
```

task 在调用线程栈上，因此函数返回前必须保证 event loop 不再访问它。后面的 condition wait 正是生命周期屏障。

### 54.3 投递并等待

```cpp
pthread_mutex_lock(&task.mutex);
scheduler->triggerEvent(controlTriggerId, &task);
while (!task.done)
{
    pthread_cond_wait(&task.cond, &task.mutex);
}
```

- 先加锁再 trigger，防止 event loop 很快完成并 signal，而调用线程还没进入等待造成丢信号；
- 使用 while 而不是 if，处理 condition variable 虚假唤醒；
- `pthread_cond_wait` 会原子释放 mutex 并睡眠，唤醒后重新获得 mutex；
- event loop 填写 result/done 后 signal；
- 返回前销毁 cond/mutex，task 才离开栈。

这是封装层中线程同步写得较正确的一处。

### 54.4 `cleanServerMediaSession()`

```cpp
rtspServer->deleteServerMediaSession(session);
server_session[i] = NULL;
nUse--;
```

live555 会关闭使用该 session 的客户端状态和所有 subsession/source/sink。数组指针必须在 delete 后立即清 null，防止后续重复删除。

## 55. `rtsp_server_unInit()`：为什么当前退出存在 UAF 风险

源码位置：`rtspServer_base.cpp:505-579`。

### 55.1 设置两个退出标志

```cpp
m_quit = 1;
m_aQuit.store(1, std::memory_order_seq_cst);
```

- `m_quit` 给 printError thread；
- atomic `m_aQuit` 给 event loop；
- 两个字段表达同一生命周期状态，增加了同步复杂度。

### 55.2 只 join 打印线程

```cpp
// pthread_join(eventLoopTid, NULL);  // 被注释
pthread_join(printErrorTid, NULL);
```

打印线程 sleep 最多 1 秒后退出，join 可以保证它不再访问 environment。

event loop 没有 join，随后马上执行：

```cpp
cleanAllServerMediaSession(pServerInfo);
deleteEventTrigger(...);
Medium::close(rtspServer);
usage_env->reclaim();
delete scheduler;
free(pRtspHandle);
```

若 event thread 还位于 `doEvent()` 或下一次循环读取 `pServeInfo->m_aQuit`，这些对象/结构体已经释放，形成 use-after-free。

正确退出需要：

1. 设置 quit；
2. trigger 一个 event 唤醒 scheduler；
3. join eventLoopTid；
4. 确认线程退出后再释放 live555 对象。

### 55.3 `cleanAllServerMediaSession()` 的提前 return

```cpp
for (i = 0; i < MAXRTSPNUM; i++)
{
    if (server_session[i] != NULL)
    {
        deleteServerMediaSession(...);
        ...
        return 0;
    }
}
```

函数名表示清理全部，但第一次命中后就 return，只清一个。上层 deinit 事先逐路调用 destroy 时通常把两路清完，所以问题被掩盖；如果 init 部分失败或出现额外 session，unInit 会残留其他 session。

### 55.4 authDB 未释放

`RTSPServer` 保存 `fAuthDB` 但其析构没有 delete；头文件也说明 caller 负责。wrapper 只在局部变量中创建 authDB，未存入 `Rtsp_Server_Info_t`，unInit 无法回收，造成每次 reboot 泄漏一份认证数据库和用户记录。

## 56. `RTSPServer::createNew()`：监听 socket 和 server 对象

源码位置：`liveMedia/RTSPServer.cpp:40-53`。

```cpp
int ourSocketIPv4 = setUpOurSocket(env, ourPort, AF_INET, dscp);
int ourSocketIPv6 = setUpOurSocket(env, ourPort, AF_INET6, dscp);
if (ourSocketIPv4 < 0 && ourSocketIPv6 < 0)
    return NULL;
```

- 分别创建 IPv4/IPv6 TCP listen socket；
- `dscp` 在 socket setup 阶段设置 IP TOS/traffic class；
- 使用逻辑 AND，只有两种地址族都失败才整体失败；
- 某些系统关闭 IPv6时 IPv6 返回负数，但 IPv4 仍可正常服务。

```cpp
return new RTSPServer(env, socket4, socket6,
                      ourPort, authDatabase,
                      reclamationSeconds, authAlgorithm);
```

构造函数把两只 listen socket 交给 `GenericMediaServer`。基类会向 scheduler 注册 socket readable handler；有新 TCP 连接时创建 `RTSPClientConnection`。

构造成员：

```cpp
fAllowStreamingRTPOverTCP(True),
fOurConnectionsUseTLS(False),
fWeServeSRTP(False),
fAuthAlgorithm(authAlgorithm)
```

这四个值准确描述产品当前能力：

- 允许 RTP interleaved over TCP；
- RTSP 控制连接不是 TLS；
- RTP 不是 SRTP；
- Digest 算法由配置指定。

## 57. `RTSPClientConnection::handleRequestBytes()`：一个 TCP 字节流怎样变成 RTSP 方法

源码位置：`RTSPServer.cpp:944-1408`。

这是 RTSP 控制面的总入口。所有 OPTIONS、DESCRIBE、SETUP、PLAY、TEARDOWN 最初都从这里分流。

### 57.1 TCP 没有消息边界

```cpp
unsigned char* ptr = &fRequestBuffer[fRequestBytesAlreadySeen];
```

一次 `recv()` 可能只收到半个 RTSP header，也可能一次收到两个 pipelined 请求。因此连接对象维护：

- 已看到字节数；
- 剩余缓冲容量；
- 上一个 CRLF 位置；
- 本次新读入数据起点。

### 57.2 异常长度关闭连接

```cpp
if (newBytesRead < 0 ||
    (unsigned)newBytesRead >= fRequestBufferBytesLeft)
{
    fIsActive = False;
    break;
}
```

- 负数表示 socket 错误/关闭；
- 新数据大于等于剩余容量意味着请求过大，无法留下终止符空间；
- 标记 inactive 后在函数尾部删除 connection，避免继续解析不可信缓冲。

### 57.3 查找 `CRLF CRLF`

```cpp
while (tmpPtr < &ptr[newBytesRead - 1])
{
    if (*tmpPtr == '\r' && *(tmpPtr + 1) == '\n')
    {
        if (tmpPtr - fLastCRLF == 2)
        {
            endOfMsg = True;
            break;
        }
        fLastCRLF = tmpPtr;
    }
    ++tmpPtr;
}
```

RTSP header 以空行结束，即两个连续 CRLF。保存上一次 CRLF 地址后，如果下一次 CRLF 相隔 2 字节，就找到了空行。

只找到 header 末尾还不一定请求完整，因为 SET_PARAMETER 等可能有 `Content-Length` body，后面还要确认 body 字节齐全。

### 57.4 临时写 `\0` 再恢复

```cpp
fLastCRLF[2] = '\0';
parseRTSPRequestString(...);
fLastCRLF[2] = '\r';
```

parser 使用 C 字符串接口，需要临时截断到 header 末尾。解析后恢复原始字节，后面计算 request size 和处理 body 时仍需要完整缓冲。

### 57.5 Content-Length 溢出检查

```cpp
if (tmpPtr + 2 + contentLength < tmpPtr + 2)
{
    contentLength = 0;
    parseSucceeded = False;
}
```

如果恶意 Content-Length 让指针加法发生整数环绕，结果地址会反而更小。这个比较用于拒绝 wraparound，防止后续把不完整缓冲当作巨大 body。

### 57.6 Session header 同时承担保活

```cpp
if (sessionIdStr[0] != '\0')
{
    clientSession = lookupClientSession(sessionIdStr);
    if (clientSession != NULL)
        clientSession->noteLiveness();
}
```

任何带有效 Session 的 RTSP 请求都会刷新 65 秒回收定时器。RTCP RR 也能刷新，因此持续播放但客户端很少发 GET_PARAMETER 时 session 仍可保持。

### 57.7 方法分派顺序

分派顺序是：

```text
URL scheme redirect
 -> OPTIONS
 -> server-wide GET/SET_PARAMETER '*'
 -> DESCRIBE
 -> SETUP
 -> session methods PLAY/PAUSE/TEARDOWN/GET/SET_PARAMETER
 -> REGISTER/DEREGISTER
 -> 405
```

OPTIONS 不一定需要 session；DESCRIBE 在 connection 层处理；SETUP 负责创建 `RTSPClientSession`；PLAY 等必须查到已有 session。

### 57.8 SETUP 为什么创建 ClientSession

```cpp
if (!requestIncludedSessionId)
{
    if (authenticationOK("SETUP", urlTotalSuffix, request))
    {
        clientSession = createNewClientSessionWithId();
    }
}
```

- 第一个 track 的 SETUP 没有 Session header，server 创建随机 session id；
- 后续 audio track SETUP 带同一个 Session，复用同一 `RTSPClientSession`；
- 认证通过后才分配 session，避免未认证请求消耗长期状态。

### 57.9 pipelined 请求

```cpp
numBytesRemaining = fRequestBytesAlreadySeen - requestSize;
resetRequestBuffer();
if (numBytesRemaining > 0)
{
    memmove(fRequestBuffer,
            &fRequestBuffer[requestSize],
            numBytesRemaining);
    newBytesRead = numBytesRemaining;
}
```

客户端可能在一个 TCP 包中连续发送两条 RTSP 请求。处理完第一条后，把剩余字节移到缓冲头并继续 do/while，不必等待下一次 socket readable。

使用 `memmove` 而不是 memcpy，因为源和目标区域重叠。

### 57.10 recursion count 的原因

DESCRIBE 生成 SDP 时 `getAuxSDPLine()` 会再次进入 event loop。新 socket 事件可能递归调用同一个 connection handler。如果内层把 connection delete，外层栈会继续使用悬空 this。

因此：

```cpp
++fRecursionCount;
...
--fRecursionCount;
if (!fIsActive)
{
    if (fRecursionCount > 0) closeSockets();
    else delete this;
}
```

只有最外层调用返回时才真正 delete，这是对嵌套 event loop 的生命周期保护。

### 57.11 RTSP-over-HTTP 分支为何存在但产品未启用

当输入/输出 socket 不同，代码会把 POST 输入按 Base64 解码，并用 session cookie 配对 GET 输出。这是 live555 通用的 RTSP-over-HTTP tunnel。

当前项目没有调用 `setUpTunnelingOverHTTP()` 配置 tunnel 端口，所以这些 parser 分支在正常 IPC 配置下不会进入。阅读通用库源码时要区分“编译存在”和“产品启用”。

## 58. `authenticationOK()`：Digest 每一项如何验证

源码位置：`RTSPServer.cpp:1494-1640`。

### 58.1 无认证数据库时直接允许

```cpp
UserAuthenticationDatabase* authDB =
    getAuthenticationDatabaseForCommand(cmdName);
if (authDB == NULL)
    return True;
```

认证开关最终表现为 RTSPServer 是否持有 authDB，而不是每个 session 单独配置。

### 58.2 为什么第一次一定失败并发 challenge

```cpp
if (fCurrentAuthenticator.nonce() == NULL)
    break;
```

第一次请求没有 server nonce，无法验证 response。代码跳到函数尾生成随机 nonce 和 401，这是标准 challenge-response 两阶段流程。

### 58.3 Authorization 字段一致性

parser 提取：

```text
username, realm, nonce, uri, response
```

随后检查：

- realm 等于 server realm；
- nonce 等于当前 connection 保存的 nonce；
- uri/response 均存在；
- username 能在 authDB 查到密码。

不直接信任客户端提交的 realm/nonce，防止用别的 challenge 生成的摘要重放。

### 58.4 服务端重算摘要

```cpp
fCurrentAuthenticator.setUsernameAndPassword(
    username, password, authDB->passwordsAreMD5());
char const* ourResponse =
    fCurrentAuthenticator.computeDigestResponse(cmdName, uri);
success = strcmp(ourResponse, response) == 0;
```

摘要绑定当前 RTSP method 和 URI。因此同一个 response 不能从 DESCRIBE 原样复制到 SETUP。

组合算法模式通过 response 长度判断：

- 32 hex 字符 -> MD5；
- 64 hex 字符 -> SHA-256。

这是一种兼容实现，但更严格的方式应解析 Authorization 中的 algorithm 参数，而不只靠长度猜测。

### 58.5 401 challenge

失败后：

```cpp
setRealmAndRandomNonce(authDB->realm());
```

再生成：

```text
WWW-Authenticate: Digest realm="...",
 nonce="...", algorithm=..., stale=false
```

组合模式发两条 WWW-Authenticate，让客户端选择 MD5 或 SHA-256。

`stale=false` 表示不是因为旧 nonce 过期，而是当前请求未通过。当前实现每次失败生成新 nonce，但没有完整的 nonce 生命周期/重放窗口管理。

## 59. `handleCmd_DESCRIBE()`：为什么它会进入编码数据路径

源码位置：`RTSPServer.cpp:534-623`。

### 59.1 拼 URL suffix

```cpp
if (urlPreSuffix[0] != '\0')
{
    strcat(urlTotalSuffix, urlPreSuffix);
    strcat(urlTotalSuffix, "/");
}
strcat(urlTotalSuffix, urlSuffix);
```

对于：

```text
rtsp://ip/Streaming/Channels/101
```

parser 可能把前后段拆开，最终重新组成与 `ServerMediaSession::streamName()` 完全相同的 key。

固定数组大小是两个 RTSP_PARAM_STRING_MAX，意图容纳两段和 `/`。代码使用 strcat，安全性依赖 parser 已限制每段长度。

### 59.2 先鉴权再 lookup

```cpp
if (!authenticationOK("DESCRIBE", urlTotalSuffix, fullRequestStr))
    return;
lookupServerMediaSession(urlTotalSuffix, completion, this);
```

未认证客户端不会触发 session lookup 和 dummy source 取帧，防止匿名请求消耗 VENC/IDR 资源。

lookup 使用 completion callback，是因为 GenericMediaServer 允许子类异步创建 session。当前已有 session，通常同步回调。

### 59.3 临时增加引用计数

```cpp
session->incrementReferenceCount();
sdpDescription = session->generateSDPDescription(...);
...
session->decrementReferenceCount();
```

生成 SDP 可能进入嵌套 event loop，期间控制线程可能请求删除 session。临时 ref 防止 session 在 SDP 构造过程中被回收。

这个引用不是“正在播放客户端数”，只是 DESCRIBE 操作期间的短期保护，结束立即减回。

### 59.4 Content-Base

```cpp
rtspURL = rtspURL(session, fClientInputSocket);
```

传入 client socket 后 `rtspURLPrefix()` 用 `getsockname()` 取得这条连接实际到达的本地接口地址。多网卡设备上，返回给客户端的 Content-Base 更可能是可达地址，而不是任意默认网卡。

响应：

```text
Content-Base: rtsp://ip:port/Streaming/Channels/101/
Content-Type: application/sdp
Content-Length: ...
```

末尾 `/` 很重要，客户端用它解析 SDP 中相对 `a=control:track1`。

### 59.5 为什么生成 SDP 会拉取真实帧

`generateSDPDescription()` 调各 subsession `sdpLines()`。H.264/H.265 的 aux SDP 必须从真实参数集生成，因此调用链进入：

```text
DESCRIBE
 -> sdpLines
 -> create dummy source/sink
 -> startPlaying
 -> source getNextFrame
 -> rtspFrameCall
 -> VENC queue
```

所以 DESCRIBE 不是纯字符串操作，它会触发媒体生产、IDR 和内存分配。

## 60. `RTSPServer::RTSPClientSession::handleCmd_SETUP()`：Transport 头怎样变成 UDP 端口或 TCP channel

核心位置：`RTSPServer.cpp:1905-2335`。

### 60.1 第一次 SETUP 创建 track state 数组

```cpp
fNumStreamStates = session->numSubsessions();
fStreamStates = new streamState[fNumStreamStates];
```

一个 RTSP session 对应一个客户端访问 `/101` 的状态，数组每项对应 video/audio track，保存：

- subsession 指针；
- TCP socket 号，UDP 时为 -1；
- `streamToken`，实际指向 `StreamState`。

### 60.2 根据 trackId 找 subsession

```cpp
for (trackNum = 0; trackNum < fNumStreamStates; ++trackNum)
{
    if (strcmp(fTrackId, subsession->trackId()) == 0)
        break;
}
```

trackId 来自 SDP `a=control:`。客户端不能自己随意写 track1/track2，必须与 DESCRIBE 返回一致。

### 60.3 重复 SETUP 同一 track

```cpp
if (token != NULL)
{
    subsession->pauseStream(...);
    unnoteTCPStreamingOnSocket(...);
    subsession->deleteStream(...);
}
```

先清理旧 transport 和 destination，再重新建立，避免同一个 client session 对一个 track 持有两套 RTP 状态。

### 60.4 解析 transport

`parseTransportHeader()` 提取：

- `RTP/AVP` 或 `RTP/AVP/TCP`；
- unicast/multicast；
- client_port；
- destination/ttl；
- interleaved RTP/RTCP channel id。

如果客户端请求 TCP 却没写 interleaved，代码自动使用当前 `fTCPStreamIdCount` 和 +1，并每个 track 增加 2。

典型分配：

```text
video RTP channel 0, RTCP 1
audio RTP channel 2, RTCP 3
```

### 60.5 调用 subsession 建立媒体状态

```cpp
subsession->getStreamParameters(
    sessionId, clientAddr,
    clientRTPPort, clientRTCPPort,
    tcpSocketNum, rtpChannelId, rtcpChannelId,
    tlsState, destinationAddress, destinationTTL,
    isMulticast, serverRTPPort, serverRTCPPort,
    streamToken);
```

这是 RTSP 控制层进入媒体/RTP 层的关键接口。它不立即发送帧，只创建或复用 source、sink、groupsock，并记录目的地。

### 60.6 UDP 响应

返回 client/server 两对端口：

```text
Transport: RTP/AVP;unicast;
 destination=<client>;
 source=<device>;
 client_port=x-y;
 server_port=a-b
```

客户端随后监听 x/y，服务器从 a/b 发送。

### 60.7 TCP 响应

```text
Transport: RTP/AVP/TCP;unicast;
 interleaved=0-1
```

没有额外 UDP 目的端口，RTP/RTCP 后续写入当前 RTSP TCP socket。

## 61. `RTSPClientSession::handleCmd_PLAY()/handleCmd_PAUSE()/handleCmd_TEARDOWN()`：控制方法怎样改变媒体状态

### 61.1 PLAY

`handleCmd_PLAY()` 先解析 Range/Scale。实时流不支持真正 seek，但基类接口仍保留通用文件播放能力。

启动每个 track：

```cpp
subsession->startStream(
    sessionId, streamToken,
    noteClientLiveness, this,
    rtpSeqNum, rtpTimestamp,
    handleAlternativeRequestByte, connection);
```

输出参数 seq/timestamp 被写入响应：

```text
RTP-Info: url=.../track1;seq=...;rtptime=...
```

客户端用它把 PLAY 响应与第一批 RTP 对齐。

### 61.2 PAUSE

遍历目标 track 调 `pauseStream()`，返回 200。项目 subsession 开启 `reuseFirstSource`，OnDemand 基类会拒绝暂停共享 source，因此它更多是协议兼容响应，不一定停止公共编码流。

### 61.3 TEARDOWN

```cpp
unnoteTCPStreamingOnSocket(...);
subsession->deleteStream(sessionId, streamToken);
fStreamStates[i].subsession = NULL;
```

- 先从 RTSPServer 的 TCP streaming database 移除 socket-track 关系；
- 再删除该客户端 destination 和引用；
- 将 subsession 指针清 null 表示本 client 不再拥有该 track。

全部 track 都清空后 `delete this`，立刻回收 `RTSPClientSession`，不必等 65 秒超时。

### 61.4 GET_PARAMETER 保活

session 内 `GET_PARAMETER` 不读取 body，只返回 200 并带 Session。由于请求处理入口已经 `noteLiveness()`，它仍能刷新超时，是很多客户端的 keepalive 方式。

## 62. `ServerMediaSession`：一个 URL 为什么能包含两个 track

源码位置：`ServerMediaSession.cpp:61-111`。

### 62.1 构造函数状态

```cpp
: Medium(env),
  streamingUsesSRTP(False),
  streamingIsEncrypted(False),
  fSubsessionsHead(NULL),
  fSubsessionsTail(NULL),
  fSubsessionCounter(0),
  fReferenceCount(0),
  fDeleteWhenUnreferenced(False)
```

- `Medium` 是 live555 可命名、可统一 close 的基础对象；
- SRTP/encrypted 明确关闭；
- subsession 用单向链表保存；
- referenceCount 统计当前 RTSP client session/临时操作引用；
- deleteWhenUnreferenced=false 表示不会只因暂时无人使用就自动删除，IPC 的 `/101`、`/102` 长期存在。

### 62.2 字符串为什么全部复制

```cpp
fStreamName = strDup(streamName == NULL ? "" : streamName);
fInfoSDPString = strDup(...);
fDescriptionSDPString = strDup(...);
```

调用方传入的是栈结构或外部缓冲，session 生命周期更长，所以必须复制。析构中对应 `delete[]`。

### 62.3 `addSubsession()`

```cpp
if (subsession->fParentSession != NULL)
    return False;
```

一个 subsession 不能同时属于两个 session，因为它内部的 track number、parent、共享 source 状态都只有一份。

链表插入：

```cpp
if (fSubsessionsTail == NULL)
    fSubsessionsHead = subsession;
else
    fSubsessionsTail->fNext = subsession;
fSubsessionsTail = subsession;
```

保持 head/tail 后，添加 track 是 O(1)。

```cpp
subsession->fParentSession = this;
subsession->fTrackNumber = ++fSubsessionCounter;
```

先递增，所以第一个 track 是 1。`trackId()` 延迟生成 `track1`、`track2`。在本项目中通常：

```text
track1 = video
track2 = audio
```

因为 `rtsp_server_create()` 总是先 add video，再 add audio。

### 62.4 subsession 析构为何递归关闭 `fNext`

```cpp
ServerMediaSubsession::~ServerMediaSubsession()
{
    delete[] fTrackId;
    Medium::close(fNext);
}
```

删除链表头会递归删除后继，`ServerMediaSession::deleteAllSubsessions()` 只需 close head。前提是每个 subsession 只属于一条链，这正是 parent 检查保证的。

## 63. `generateSDPDescription()`：SDP 字符串逐层怎样拼出来

源码位置：`ServerMediaSession.cpp:216-335`。

### 63.1 选择 SDP 地址

```cpp
if (addressFamily == AF_INET)
    ourAddress = ourIPv4Address(envir());
else
    ourAddress = ourIPv6Address(envir());
```

DESCRIBE connection 的地址族决定 SDP 使用 `IP4` 还是 `IP6`。这与 RTP 最终 UDP/TCP transport 协商是两个层次。

### 63.2 为什么先遍历一次统计长度

```cpp
for (subsession = head; subsession != NULL; ...)
{
    char const* lines = subsession->sdpLines(addressFamily);
    if (lines == NULL) continue;
    sdpLength += strlen(lines);
}
```

调用 `sdpLines()` 不只是取缓存：第一次会创建 dummy source/sink，H.264/H.265 甚至等待参数集。因此必须先让每个 track 完成初始化，再知道最终字符串长度。

如果所有 subsession 都返回 null，session 没有可用媒体，DESCRIBE 返回 404/格式错误。

### 63.3 实时流 Range

```cpp
float dur = duration();
if (dur == 0.0)
    rangeLine = "a=range:npt=now-\r\n";
```

每个项目 subsession 默认 duration=0，代表无限实时流，不是零秒文件。

### 63.4 SDP session prefix

```text
v=0
o=- <creation-sec><usec> 1 IN IP4 <address>
s=<description>
i=<info>
t=0 0
a=tool:<live555 version>
a=type:broadcast
a=control:*
```

- creation time 组合成 session id；
- version 固定 1，注释说明参数修改时理论应递增，但当前 reboot 会创建新 session；
- `a=control:*` 表示 PLAY 可对整个 session 聚合操作；
- `t=0 0` 表示永久可用，而不是预约时间段。

### 63.5 第二遍追加 track

第一次遍历会触发副作用并统计长度，分配总缓冲后第二遍把每个 track 字符串 append。

源码额外加 1000 字节余量，因为两次调用之间 subsession 的 SDP 长度理论可能变化。它是防御性余量，不是严格的长度证明。

## 64. `OnDemandServerMediaSubsession::sdpLines()`：dummy source/sink 为什么创建后又销毁

源码位置：`OnDemandServerMediaSubsession.cpp:61-90`。

```cpp
if (fSDPLines == NULL)
{
    FramedSource* inputSource =
        createNewStreamSource(0, estBitrate);
```

- SDP 第一次生成时才创建，结果缓存到 `fSDPLines`；
- clientSessionId 传 0，因为它不是实际客户端媒体流；
- codec 子类通过 virtual function 创建 H264/H265/AAC 等 source。

```cpp
Groupsock* dummyGroupsock =
    createGroupsock(nullAddress(addressFamily), 0);
```

- SDP 需要一个 RTP sink 来提供 payload name/clock/fmtp；
- 但 DESCRIBE 不应真的占用正式 RTP server port；
- null address + port 0 创建仅供参数查询的 groupsock。

```cpp
unsigned char rtpPayloadType = 96 + trackNumber() - 1;
RTPSink* dummyRTPSink = createNewRTPSink(
    dummyGroupsock, rtpPayloadType, inputSource);
```

动态 payload 分配与真实 SETUP 时一致，确保 SDP 和后续 RTP 包 PT 相同。

```cpp
setSDPLinesFromRTPSink(dummyRTPSink, inputSource, estBitrate);
Medium::close(dummyRTPSink);
delete dummyGroupsock;
closeStreamSource(inputSource);
```

生成字符串后 dummy 对象全部销毁。H264/H265 source 析构会发 STOP callback，因此 DESCRIBE 阶段可出现短暂 START→STOP。

缓存 `fSDPLines` 后，后续 DESCRIBE 不再读取最新参数集。如果运行中 codec/profile/resolution 改变，必须销毁并重建 subsession，也就是项目选择 RTSP reboot 的原因。

## 65. `setSDPLinesFromRTPSink()`：每个 `m=`、`a=` 字段从哪里来

源码位置：`OnDemandServerMediaSubsession.cpp:435-488`。

```cpp
char const* mediaType = rtpSink->sdpMediaType();
unsigned char payloadType = rtpSink->rtpPayloadType();
char* rtpmapLine = rtpSink->rtpmapLine();
char const* auxSDPLine = getAuxSDPLine(rtpSink, inputSource);
```

- mediaType 返回 video/audio；
- payload type 来自 dummy sink 构造参数或 codec 静态 PT；
- rtpmap 包含 codec name、clock rate、channels；
- aux line 由 codec 定制，例如 H264 fmtp 或 AAC fmtp。

格式模板：

```cpp
"m=%s %u RTP/%sAVP %d\r\n"
"c=IN %s %s\r\n"
"b=AS:%u\r\n"
"%s" // rtpmap
"%s" // key management
"%s" // rtcp-mux
"%s" // range
"%s" // aux fmtp
"a=control:%s\r\n";
```

项目未启用 SRTP，所以协议是 `RTP/AVP`，不会插入 S 形成 `RTP/SAVP`。也没有开启 RTP/RTCP multiplex，所以通常无 `a=rtcp-mux`。

SDP 中 `m=` 端口在 on-demand unicast 描述里可能是 dummy port 0；真正端口在 SETUP Transport 响应协商，客户端不能只按 SDP m= 端口发送。

## 66. `getStreamParameters()`：SETUP 后 source、sink、socket 的创建过程

源码位置：`OnDemandServerMediaSubsession.cpp:93-216`。

### 66.1 destination 默认使用 RTSP 客户端地址

```cpp
if (addressIsNull(destinationAddress))
    destinationAddress = clientAddress;
isMulticast = False;
```

项目没有开启允许客户端任意指定 destination 的宏，避免服务器被利用向第三方地址发送大量 UDP，形成反射攻击。

### 66.2 `reuseFirstSource`

```cpp
if (fLastStreamToken != NULL && fReuseFirstSource)
{
    serverRTPPort = last->serverRTPPort();
    serverRTCPPort = last->serverRTCPPort();
    ++last->referenceCount();
    streamToken = fLastStreamToken;
}
```

第二个客户端不再创建 source/sink/socket，而是复用第一份 `StreamState` 并加引用。这样同一编码数据只从 queue pop 一次，再由 Groupsock/RTPInterface 发给多个 destination。

### 66.3 第一个客户端创建 media source

```cpp
FramedSource* mediaSource =
    createNewStreamSource(clientSessionId, streamBitrate);
```

视频子类返回 `H264VideoStreamFramer(H264_Video_Source)` 等组合；音频子类直接返回自定义 audio source。

### 66.4 UDP 端口选择

```cpp
for (serverPortNum = fInitialPortNum; ; ++serverPortNum)
{
    serverRTPPort = serverPortNum;
    rtpGroupsock = createGroupsock(..., serverRTPPort);
    if (rtp socket failed) continue;

    serverRTCPPort = ++serverPortNum;
    rtcpGroupsock = createGroupsock(..., serverRTCPPort);
    if (rtcp socket failed) retry;
    break;
}
```

构造函数把 initial port 调整为偶数，因此 RTP 偶数、RTCP 相邻奇数。`NoReuse` 临时对象防止错误复用已占用端口。

即使客户端选 TCP，代码仍可能创建 UDP groupsock 作为 sink 基础对象，随后 removeAllDestinations，真正发送改走 stream socket。

### 66.5 创建 RTP sink

```cpp
rtpSink = createNewRTPSink(
    rtpGroupsock, payloadType, mediaSource);
```

virtual dispatch 根据 codec 选择 H264VideoRTPSink、MPEG4GenericRTPSink、SimpleRTPSink 等。

### 66.6 socket send buffer

```cpp
unsigned rtpBufSize = streamBitrate * 25 / 2;
if (rtpBufSize < 50 * 1024)
    rtpBufSize = 50 * 1024;
increaseSendBufferTo(..., rtpBufSize);
```

streamBitrate 单位是 kbit/s：

```text
kbit/s × 1000 / 8 × 0.1s = kbit/s × 12.5 bytes
```

即 `*25/2`。至少 50 KiB，吸收短时调度抖动。

本项目 H264 source 把 estBitrate 写成 2,000,000，若按 kbit/s 解释会请求约 25 MB socket buffer并生成夸张 `b=AS`。内核会设上限，但参数明显需要核对单位。

### 66.7 创建 StreamState

```cpp
streamToken = fLastStreamToken =
    new StreamState(*this,
                    serverRTPPort, serverRTCPPort,
                    rtpSink, udpSink,
                    streamBitrate, mediaSource,
                    rtpGroupsock, rtcpGroupsock);
```

`StreamState` 是 source/sink/socket/RTCP 的统一所有者，也是多个客户端共享的引用计数对象。

### 66.8 每个客户端仍有独立 destination

```cpp
if (tcpSocketNum < 0)
    destinations = new Destinations(addr, rtpPort, rtcpPort);
else
    destinations = new Destinations(tcpSocketNum,
                                    rtpChannelId,
                                    rtcpChannelId,
                                    tlsState);
fDestinationsHashTable->Add((char const*)clientSessionId,
                            destinations);
```

共享的是媒体生产和 RTP sink，不共享目的地。哈希 key 是 client session id，被强制转成 char pointer 作为 one-word key 使用，不是字符串地址内容。

## 67. `StreamState::startPlaying()`：真正开始 RTP/RTCP 的位置

源码位置：`OnDemandServerMediaSubsession.cpp:523-583`。

### 67.1 第一次 PLAY 创建 RTCP

```cpp
if (fRTCPInstance == NULL && fRTPSink != NULL)
{
    fRTCPInstance = fMaster.createRTCP(
        fRTCPgs, fTotalBW, cname, fRTPSink);
}
```

RTCP 依赖 RTP sink 的 SSRC、timestamp 和发送统计，因此在 sink 已存在后创建。一个共享 StreamState 只有一个 RTCPInstance，向所有 destination 发送相同 sender report。

### 67.2 TCP destination

```cpp
fRTPSink->addStreamSocket(tcpSocketNum,
                          rtpChannelId,
                          tlsState);
fRTCPInstance->addStreamSocket(tcpSocketNum,
                               rtcpChannelId,
                               tlsState);
```

RTP/RTCP 不再通过 UDP destination，而是添加到同一 RTSP TCP socket 的不同 interleaved channel。

注册 alternative byte handler 是为了 sink 发送媒体期间仍能从该 socket 读取 RTSP 请求，不让 RTP-over-TCP 抢占控制连接。

### 67.3 UDP destination

```cpp
fRTPgs->addDestination(addr, rtpPort, clientSessionId);
fRTCPgs->addDestination(addr, rtcpPort, clientSessionId);
```

Groupsock 可保存多个 destination。一份 RTP packet build 完成后发送给所有地址。

### 67.4 先发 RTCP SR 再发第一包 RTP

```cpp
fRTCPInstance->sendReport();
```

注释称为 hack：客户端尽早拿到 RTP timestamp 与 NTP 的对应关系，音视频同步不必等待正常 RTCP 周期。

### 67.5 sink 只启动一次

```cpp
if (!fAreCurrentlyPlaying && fMediaSource != NULL)
{
    fRTPSink->startPlaying(*fMediaSource, ...);
    fAreCurrentlyPlaying = True;
}
```

第二个客户端只添加 destination，不会第二次 start source。否则两个 pull 链同时从一个业务 queue pop，会让不同客户端收到交错的 NAL。

## 68. `OnDemandServerMediaSubsession::deleteStream()`、`StreamState::endPlaying()`、`StreamState::reclaim()`：多客户端离开时为什么不能随便发 BYE

### 68.1 删除单个 destination

`deleteStream()` 从哈希表移除该 client 的 `Destinations`，调用：

```cpp
streamState->endPlaying(destinations, clientSessionId);
```

UDP 模式移除 Groupsock destination；TCP 模式移除 socket/channel。其他客户端仍保留。

### 68.2 单客户端 TEARDOWN 不发公共 BYE

源码中显式 `#if 0` 禁用 `fRTCPInstance->sendBYE()`。原因是共享 RTCP SSRC 对所有客户端相同，一个客户端离开时向所有 destination 发送 BYE 会让其他客户端认为整个流结束。

### 68.3 引用计数

```cpp
if (streamState->referenceCount() > 0)
    --referenceCount;
if (referenceCount == 0)
{
    delete streamState;
    streamToken = NULL;
}
```

最后一个客户端离开才销毁公共 source/sink/socket。

### 68.4 `reclaim()` 顺序

```cpp
Medium::close(fRTCPInstance); // 发送 RTCP BYE
Medium::close(fRTPSink);
Medium::close(fUDPSink);
fMaster.closeStreamSource(fMediaSource);
delete fRTPgs;
if (fRTCPgs != fRTPgs) delete fRTCPgs;
```

- 先关 RTCP，使它还能读取有效 sink/source 统计并发送最终 BYE；
- 再关 sink，停止继续向 source 请求帧；
- 再关 source，触发自定义 source 析构和 STOP callback；
- 最后删除底层 socket groupsock；
- RTP/RTCP mux 时两者可能是同一指针，条件避免 double delete。

## 69. `RTPSink`：sequence、SSRC、timestamp 为什么随机起步

源码位置：`RTPSink.cpp:47-114`。

### 69.1 构造状态

```cpp
fSeqNo = (u_int16_t)our_random();
fSSRC = our_random32();
fTimestampBase = our_random32();
```

RTP 不要求 sequence/timestamp 从 0 开始，随机初值可以：

- 降低不同会话或重启后被误认为同一连续流的概率；
- 让简单的已知明文/流量分析更困难；
- 遵循 RTP 常见实现习惯。

SSRC 标识一个同步源。共享 source 模式下，多个客户端收到同一个 SSRC，因为它们订阅同一个 RTP sink。

### 69.2 wall-clock 转 RTP timestamp

```cpp
u_int32_t timestampIncrement =
    fTimestampFrequency * tv.tv_sec;
timestampIncrement +=
    fTimestampFrequency * (tv.tv_usec / 1000000.0) + 0.5;
return fTimestampBase + timestampIncrement;
```

- 视频 frequency=90000；
- AAC/G711/G726 frequency=采样率；
- `+0.5` 做四舍五入，减少浮点转整数系统性截断；
- 32 位自然回绕是 RTP 协议允许的，客户端应按模 2^32 计算差值。

这里使用 `timeval` wall clock。如果系统校时跳变，RTP timestamp 也可能突变。理想实现应从稳定媒体 PTS 映射。

### 69.3 `presetNextTimestamp()`

PLAY 响应需要在真正第一包发送前给出 `RTP-Info rtptime`。该函数按当前时间预计算一个 timestamp，并调整 base，使下一次真实 frame 转换得到相同起点。

若 sink 已有多个 destination，不再调整 base，否则新客户端加入会让正在播放客户端的 timestamp 流突然改变。

## 70. `MultiFramedRTPSink::buildAndSendPacket()`：12 字节 RTP 头逐位写入

源码位置：`MultiFramedRTPSink.cpp:174-202`。

```cpp
unsigned rtpHdr = 0x80000000;
```

最高两位写 RTP version=2。Padding、Extension、CSRC count 初始均为 0，Marker 默认 0。

```cpp
rtpHdr |= (fRTPPayloadType << 16);
rtpHdr |= fSeqNo;
fOutBuf->enqueueWord(rtpHdr);
```

- payload type 位于第二个字节低 7 位；
- sequence 位于低 16 位；
- Marker 稍后由 codec-specific handler 设置；
- enqueueWord 按网络字节序写入。

```cpp
fTimestampPosition = fOutBuf->curPacketSize();
fOutBuf->skipBytes(4);
```

时间戳取决于第一帧 presentation time，此时还没从 source 取到 frame，所以先留 4 字节孔，之后 `setTimestamp()` 回填。

```cpp
fOutBuf->enqueueWord(SSRC());
```

到这里形成固定 12 字节 RTP header。

codec 还可预留 special header，例如 JPEG RTP header；AAC AU header 更可能属于 frame-specific/special payload 处理。

## 71. `MultiFramedRTPSink::packFrame()/afterGettingFrame1()`：异步拉帧与 packet 聚合

### 71.1 source callback 不是同步返回值

```cpp
fSource->getNextFrame(
    fOutBuf->curPtr(),
    fOutBuf->totalBytesAvailable(),
    afterGettingFrame, this,
    ourHandleClosure, this);
```

`getNextFrame()` 注册目标地址和完成回调。自定义 source 可能 10 ms 后才调用 `afterGetting`，所以 `packFrame()` 本身先返回 event loop。

这就是 live555 的异步 pull 模型：没有阻塞等待 frame。

### 71.2 truncated 数据

如果 source 报告 `numTruncatedBytes>0`，表示目标缓冲不够，尾部已经丢弃。日志建议增加全局 OutPacketBuffer max。

但本项目的 `rtspFrameCall()` 在 memcpy 前不知道 capacity，若 frame 已经超限，可能先越界而不是优雅地产生 truncated。因此上层 callback ABI 应补 capacity。

### 71.3 overflow 不等于 input truncation

- input truncation：一帧大于 source 接收缓冲，数据永久丢失；
- packet overflow：完整帧已读入，但当前 RTP packet 放不下，保存到 `OutPacketBuffer` overflow 区，下个 packet 继续使用。

两者名字相似，但后果完全不同。

### 71.4 是否允许同包多帧

```cpp
if (fNumFramesUsedSoFar > 0 &&
    !frameCanAppearAfterPacketStart(...))
{
    setOverflowData(...);
}
```

H264/H265 sink 返回 false，因此每个 NAL/FU 从新 RTP 包开始。G711 SimpleRTPSink 的构造参数也禁止多帧聚合。其他格式可在一个 RTP packet 填多个小 frame。

### 71.5 duration 何时累计

```cpp
if (overflowBytes == 0)
{
    fNextSendTime += durationInMicroseconds;
}
```

一帧被拆片时，只在最后一片完成后累计一次 duration，避免每个 FU 分片都把发送时间向后推一帧。

项目 source 在队列积压时把 duration=0，sink 就会马上调度下一包，以尽快追上实时点。

## 72. `sendPacketIfNecessary()` 与 `RTPInterface`：UDP/TCP 最终写 socket

### 72.1 发送并更新统计

```cpp
fRTPInterface.sendPacket(packet, packetSize);
++fPacketCount;
fTotalOctetCount += packetSize;
fOctetCount += payloadBytes;
++fSeqNo;
```

- packetCount/octetCount 进入 RTCP SR；
- totalOctetCount 含 RTP header，octetCount 排除 header/format header；
- sequence 每发一个 RTP packet 增加，不是每幅图像增加；
- 一个大 IDR 拆 100 个 FU 包，sequence 会连续增加 100。

当前代码即使 `sendPacket()` 返回 false，仍更新 packet count 和 seq。协议序列保持前进，客户端能通过缺口观察丢包，但 RTCP sender octet count 会包含实际未成功发送的数据。

### 72.2 pacing

```cpp
uSecondsToGo = fNextSendTime - timeNow;
if (uSecondsToGo < 0) uSecondsToGo = 0;
scheduleDelayedTask(uSecondsToGo, sendNext, this);
```

如果已经落后于计划时间，不再 sleep，立即发送追赶。当前源码把秒差先转 int64 再乘 1,000,000，修复 int 乘法溢出。

### 72.3 `RTPInterface::sendPacket()` 同时支持 UDP 和 TCP 列表

```cpp
if (!fGS->output(...)) success = False;
for (tcpStreamRecord* stream = fTCPStreams; ...)
    sendRTPorRTCPPacketOverTCP(...);
```

同一 RTP sink 可以同时有 UDP destination 和多个 TCP interleaved 客户端。Groupsock output 会对 UDP destination 列表发送；随后遍历 TCP records。

### 72.4 TCP framing header

```cpp
framingHeader[0] = '$';
framingHeader[1] = streamChannelId;
framingHeader[2] = packetSize >> 8;
framingHeader[3] = packetSize & 0xFF;
```

长度是 16-bit network order，因此单个 interleaved RTP/RTCP packet 最大 65535 字节。项目 RTP 包约 1452 payload，不会接近上限。

先 send 4 字节 header，再 send packet body。若 body 只发一部分，TCP 字节流就处于不完整 framing 状态，无法简单跳过，所以代码尝试短暂改成 blocking 补发。

### 72.5 500 ms blocking fallback

```cpp
makeSocketBlocking(socketNum, 500);
send(remaining);
makeSocketNonBlocking(socketNum);
```

目的：非阻塞 send 遇到 EAGAIN/部分写时，尽量保证当前 `$+length+packet` 完整。

代价：live555 event loop 最多被一个慢 TCP 客户端阻塞约 500 ms。在此期间其他客户端的 RTSP、RTP、RTCP 都不能调度。共享服务器有多个慢客户端时，尾延迟会明显增加。

补发失败后移除该 stream socket，避免继续在已破坏 framing 的连接上发送。

## 73. `RTCPInstance`：SR、RR、SDES、BYE 的代码落点

### 73.1 `sendReport()`

```cpp
if (!addReport()) return;
addSDES();
sendBuiltPacket();
```

一个 compound RTCP packet 至少包含 SR/RR，再带 SDES CNAME。

服务器有 RTP sink，所以 `addReport()` 选择 `addSR()`；接收端/纯 source 才发 RR。

### 73.2 Sender Report

`addSR()` 写入：

- sender SSRC；
- 当前 NTP timestamp；
- 与之对应的 RTP timestamp；
- RTP packet count；
- RTP payload octet count；
- 可选 reception report blocks。

NTP 与 RTP 的同包映射是音画同步核心。客户端不能直接比较 video 90k timestamp 和 audio 16k timestamp，只能分别通过 SR 映射到共同 NTP 时间轴。

### 73.3 Receiver Report 解析

`processIncomingReport()` 检查 RTCP version、packet type 和长度，再处理 PT=200 SR、201 RR、202 SDES、203 BYE。

RR report block 中：

- fraction/cumulative lost 更新丢包统计；
- highest sequence 判断接收进度；
- jitter 反映到达间隔波动；
- LSR/DLSR 可估算 RTT。

```cpp
transmissionStats.noteIncomingRR(...);
noteArrivingRR(...);
```

第一行保存统计，第二行触发 liveness handler，防止正在正常接收但没发 RTSP keepalive 的客户端被回收。

### 73.4 周期调度

RTCP 发送间隔不是固定 5 秒常量，而是根据 session bandwidth、成员数、sender/receiver 比例和随机化算法计算，避免大量参与者同步发送控制包。

本项目 `fTotalBW` 来自 subsession estimated bitrate，所以错误的 2,000,000 kbit/s 估值也会影响 RTCP interval 计算。

### 73.5 BYE

`RTCPInstance` 析构先 `sendBYE()`。BYE compound packet仍先带 SR/RR，再添加 BYE SSRC 和可选 reason，符合 RTCP compound 规则。

共享 source 时只有最后一个 destination 结束并销毁公共 RTCPInstance 才适合发送 BYE。

## 74. H.264/H.265 `ServerMediaSubsession`：为什么 source 外面还要包一层 framer

源码位置：`h264_server_subsession.cpp`、`h265_server_subsession.cpp`。

### 74.1 构造时固定 `reuseFirstSource=True`

```cpp
H264_Server_Subsession(...)
    : OnDemandServerMediaSubsession(env, True)
```

第二参数无条件 True，即使 createNew 的上层参数不同，也强制所有客户端共享一个 source。

构造函数把 `Video_Source_Info_t` memcpy 到成员。这个结构只含函数指针、void* 和简单字段，按值复制可保持 ABI 稳定。

### 74.2 创建 source

```cpp
m_video_source = new H264_Video_Source(envir(), m_pSouceInfo);
return H264VideoStreamFramer::createNew(envir(), m_video_source);
```

为什么不是直接返回自定义 source：

- 自定义 source 交付的是 Annex-B 字节流块/pack；
- RTP sink 需要一个个无起始码的 NAL；
- 还需要识别 Access Unit 结束以设置 RTP Marker；
- 还要缓存 SPS/PPS/VPS 生成 SDP；
- `H264VideoStreamFramer/H265VideoStreamFramer` 承担这些 codec 语义。

组合关系：

```text
H264_Video_Source         原始 Annex-B chunk source
        ↓
H264VideoStreamFramer     NAL/AU parser + parameter cache
        ↓
H264VideoRTPSink          RTP payload/fragmenter
```

### 74.3 estimated bitrate

H.264 代码设置 `estBitrate=2000000`，H.265 设置 20000。OnDemand 基类明确按 kbps 使用，因此 H.264 值很可能多了 1000 倍。

它不仅影响 SDP `b=AS`，还影响：

- RTP socket send buffer；
- RTCP bandwidth 和发送周期；
- 客户端对流带宽的预估。

### 74.4 创建 RTP sink

```cpp
return H264VideoRTPSink::createNew(
    envir(), rtpGroupsock, dynamicPayloadType);
```

sink 不需要直接保存 inputSource，因为 `continuePlaying()` 时基类 `fSource` 已由 `MediaSink::startPlaying()` 设置，随后内部再插入 fragmenter。

## 75. `getAuxSDPLine()`：参数集等待循环逐句解释

### 75.1 已缓存直接返回

```cpp
if (fAuxSDPLine != NULL)
    return fAuxSDPLine;
```

同一个 subsession 的后续 DESCRIBE 不再触发 VENC。优点是快，缺点是编码参数运行中变化必须重建 RTSP。

### 75.2 dummy sink 只启动一次

```cpp
if (fDummyRTPSink == NULL)
{
    fDoneFlag = 0;
    fDummyRTPSink = rtpSink;
    fDummyRTPSink->startPlaying(*inputSource,
                                afterPlayingDummy,
                                this);
    checkForAuxSDPLine(this);
}
```

并发两个 DESCRIBE 时第二个看到 dummy sink 非空，不再启动第二条 pull 链，而是共同等待同一个 done flag。

### 75.3 100 ms 轮询

```cpp
if (fDummyRTPSink->auxSDPLine() != NULL)
{
    fAuxSDPLine = strDup(dasl);
    fDummyRTPSink = NULL;
    setDoneFlag();
}
else
{
    scheduleDelayedTask(100000, checkForAuxSDPLine, this);
}
```

RTP sink 的 aux line 在 framer 收齐参数集后才非空。100 ms 避免 busy loop，但会让参数集已到达后的 SDP 最多额外等待约 100 ms。

当前没有总超时。如果 VENC 永远不输出参数集，DESCRIBE 的嵌套 `doEventLoop(&fDoneFlag)` 可以长期不返回。

### 75.4 嵌套 event loop 为什么不是线程阻塞

```cpp
envir().taskScheduler().doEventLoop(&fDoneFlag);
```

它阻塞当前 DESCRIBE 调用栈，但内部仍继续调度 socket、source 和定时任务，因此 VENC queue 的取帧 callback 能执行。

代价是代码重入：其他客户端请求可能在这个嵌套循环中处理，所以前面 `fRecursionCount` 才必要。

## 76. `H264_Video_Source/H265_Video_Source`：从队列块到 FramedSource frame

### 76.1 `doGetNextFrame()` 只调度，不直接取

```cpp
m_pToken = taskScheduler().scheduleDelayedTask(
    m_toDelay, getNextFrame, this);
```

live555 要求 source 不应在 `doGetNextFrame()` 中形成深递归同步链。即使 delay=0，也通过 scheduler 在事件循环下一轮执行。

初始 `m_toDelay=10000` 微秒，队列空时每 10 ms 重试；成功后设为 0。

### 76.2 准备 `Fream_Info_t`

```cpp
m_stFrame.frameSize = 0;
m_stFrame.data = fTo;
m_stFrame.type = VIDEO_TYPE;
m_stFrame.param = m_stSourceInfo.videoindex;
m_stSourceInfo.dataGetfun(&m_stFrame);
```

- `fTo` 和 `fMaxSize` 由下游 framer/sink 在 `getNextFrame` 时设置；
- frameSize 先清零，防止 callback 没有数据时残留上一次长度；
- type 决定 `rtspFrameCall` 走 videoQueue；
- param 找到正确主/子队列。

### 76.3 空数据重试

```cpp
if (frameSize <= 4)
{
    m_toDelay = 10000;
    fFrameSize = 0;
    doGetNextFrame();
    return;
}
```

长度 4 以下不足以形成“起始码 + NAL”，被视为无有效数据。函数不能调用 `afterGetting`，否则下游会把空 frame 当真实 NAL；而是重新调度。

### 76.4 fps 更新

```cpp
if (m_stFrame.fFps > 0 && m_stFrame.fFps != m_fFps)
{
    m_fFps = m_stFrame.fFps;
    m_fUsecPerFrame = 1000000.0f / m_fFps;
}
```

使用 float 直接 `!=` 可能因微小误差频繁更新，不过配置枚举转换通常是稳定离散值。

### 76.5 H.264 presentation time

```cpp
if (fPresentationTime == 0 ||
    fPresentationTime == oldPresentationTime)
{
    gettimeofday(&fPresentationTime, NULL);
}
```

如果上游没有提供新 PTS，就用当前墙钟。H.264 source 每次 callback 能获得非零时间。

H.265 source 当前没有对应代码，直接 `afterGetting`。它依赖 framer 内部 `setPresentationTime()` 和继承状态，值得特别测试。

### 76.6 queue 积压时 duration=0

```cpp
fDurationInMicroseconds = m_fUsecPerFrame;
if (videolistsize >= 2)
    fDurationInMicroseconds = 0;
```

这不是改变 RTP timestamp，而是改变 sink 的发送调度时间。presentation time 仍决定 RTP timestamp；duration 主要决定何时请求/发送下一项。

### 76.7 小于 1 fps 时也置 0

一幅图像会拆成多个 NAL。如果每个 NAL 都带 2 秒 duration，sink 会把 SPS、PPS、SEI、IDR 各累计一次。置 0 后让 framer 在 Access Unit 级推进 presentation time。

### 76.8 `afterGetting(this)`

它调用先前注册的 completion callback，把：

- fFrameSize；
- fNumTruncatedBytes；
- fPresentationTime；
- fDurationInMicroseconds；

传给 `H264or5VideoStreamParser` 或 RTP sink。调用后下游可能立刻再次请求 frame，因此 source 成员状态必须已经全部更新。

## 77. `H264or5VideoStreamParser::parse()`：Annex-B 怎样切成 NAL

源码位置：`H264or5VideoStreamFramer.cpp:1218-1663`。

### 77.1 第一条起始码

```cpp
while (test4Bytes() != 0x00000001)
{
    get1Byte();
    setParseState();
}
skipBytes(4);
```

parser 会丢弃第一条 4 字节起始码之前的垃圾数据。`setParseState()` 保存进度，若输入缓冲暂时不够并通过异常中断，下次可从正确位置继续。

它要求流的第一条起始码是 4 字节；找到第一条后，后续同时识别 4 字节 `00000001` 和 3 字节 `000001`。

### 77.2 高效扫描下一起始码

```cpp
if ((next4Bytes & 0xFF) > 1)
{
    save4Bytes(next4Bytes);
    skipBytes(4);
}
else
{
    saveByte(next4Bytes >> 24);
    skipBytes(1);
}
```

如果 4 字节窗口最后一字节大于 1，窗口内部不可能出现 `00 00 01`，可一次复制 4 字节；接近 0/1 时逐字节推进，避免跨窗口漏掉起始码。这是 parser 的性能优化。

### 77.3 输出不包含起始码

`fOutputStartCodeSize` 决定是否把 start code 放进输出。RTP sink 使用的 framer 通常输出纯 NAL payload，因为 RTP H264/H265 不携带 Annex-B start code。

### 77.4 保存 VPS/SPS/PPS

```cpp
if (isVPS(type)) saveCopyOfVPS(...);
else if (isSPS(type)) saveCopyOfSPS(...);
else if (isPPS(type)) saveCopyOfPPS(...);
```

copy 排除 start code，后续 aux SDP 直接 Base64 编码 NAL header+RBSP。

VPS/SPS 还会解析 VUI timing：

```text
frameRate = time_scale /
            (DeltaTfiDivisor * num_units_in_tick)
```

H.264/H.265 的 divisor 规则不同，封装在 parser 的 HNumber 分支。

### 77.5 emulation prevention byte

SPS/VPS bit parser 前会移除 `00 00 03` 中的 03。编码器插入 03 防止 RBSP 内误出现 Annex-B 起始码；解析 Exp-Golomb/VUI 时必须恢复原始 RBSP。

`removeH264or5EmulationBytes()` 遇到 `00 00 03` 时输出两个 00、跳过 03。

### 77.6 判断 Access Unit 结束

Marker 必须标在一幅图像最后一个 RTP packet。parser 会 look-ahead 下一 NAL：

- EOF/结束 NAL：当前结束 AU；
- 下一 NAL 是 VCL 且 first-slice bit=1：当前结束 AU；
- 下一 NAL 是通常开始新 AU 的 AUD/SPS 等：当前结束 AU；
- 否则当前仍属于同一 AU。

当前定制代码只有当前 NAL 是 VCL 时才设置 `fPictureEndMarker=True`，避免 SPS/PPS/SEI 被误标成图像结束。

### 77.7 presentation time 推进

一旦 AU 结束：

```cpp
nextPT = currentPresentationTime;
nextPT += 1.0 / safeFrameRate;
```

这使同一 AU 内多个 NAL 共用 presentation time，下一幅图像才增加一个 frame interval。

当前代码包含低帧率和墙钟校正的项目定制。需要特别注意：

- 函数内的 `static int accumulatedStreamTimeMs` 会被所有 parser 实例、主子通道共享，存在明确的跨流串扰和 event-loop 重入状态污染；
- `usingSource()->fPresentationTime = usingSource()->fPresentationTime;` 是自赋值，对低于 1 fps 实际没有改变状态；
- H.265 上游未初始化 presentation time 时，单靠自赋值不能修复；
- 使用 gettimeofday 做校正会受系统时间调整影响。

## 78. `H264VideoRTPSink/H265VideoRTPSink::auxSDPLine()`：fmtp 怎样生成

### 78.1 H.264

sink 从 framer 取得 SPS/PPS，若任一为空返回 null，让 subsession 继续轮询。

```text
profile-level-id = SPS[1..3]
sprop-parameter-sets = Base64(SPS),Base64(PPS)
packetization-mode=1
```

`packetization-mode=1` 表示 non-interleaved 模式，允许 single NAL、STAP-A、FU-A 等；当前 fragmenter主要用 single NAL/FU-A。

### 78.2 H.265

需要 VPS/SPS/PPS 全部存在。除了三个 Base64 参数集，还从 profile tier level 解析：

- profile-space；
- profile-id；
- tier-flag；
- level-id；
- interop constraints。

如果参数集变化但 fAuxSDPLine 已缓存，客户端仍拿旧 fmtp。因此 codec 属性变更必须重建 session。

## 79. `H264or5Fragmenter::doGetNextFrame()`：大 NAL 如何变成 FU

源码位置：`H264or5VideoRTPSink.cpp:213-325`。

### 79.1 为什么内部缓冲前面多留 1 字节

```cpp
fInputBufferSize = inputBufferMax + 1;
fInputSource->getNextFrame(&fInputBuffer[1], ...);
```

下标 0 预留给 H.264 FU indicator。H.265 分片时也会重用 NAL header 附近的字节构造 3 字节 FU 头。

### 79.2 小 NAL

```cpp
if (nalSize <= fMaxSize)
{
    memmove(fTo, &fInputBuffer[1], nalSize);
    fFrameSize = nalSize;
}
```

直接作为 single NAL RTP payload，不包含 Annex-B start code。

### 79.3 H.264 第一片

```cpp
fInputBuffer[0] = (nalHeader & 0xE0) | 28;
fInputBuffer[1] = 0x80 | (nalHeader & 0x1F);
```

- FU indicator 保留 F/NRI，高 5 位 type 改为 28；
- FU header 保留原 type，设置 S=1；
- 原 NAL header 不直接作为 payload，而被拆进这两个字段。

### 79.4 H.265 第一片

```cpp
nalType = (header0 & 0x7E) >> 1;
payloadHeader0 = (header0 & 0x81) | (49 << 1);
payloadHeader1 = originalHeader1;
fuHeader = 0x80 | nalType;
```

type 49 表示 H.265 FU；保留 forbidden bit、layer id 和 temporal id 相关字段。

### 79.5 中间片和最后一片

中间片清 S；若剩余数据可在本包发完，设置 E=1。`fLastFragmentCompletedNALUnit` 让 sink 知道当前 RTP 包是否结束一个 NAL。

`fCurDataOffset` 每次增加“实际 NAL 数据字节”，减去重复插入的 FU header 数，避免跳过或重复原始数据。

## 80. `doSpecialFrameHandling()`：RTP Marker 与同帧 timestamp

```cpp
if (lastFragmentCompletedNALUnit() &&
    framerSource->pictureEndMarker())
{
    setMarkerBit();
    framerSource->pictureEndMarker() = False;
}
```

Marker 只有两个条件同时满足：

1. 当前 RTP 包结束了一个 NAL；
2. 该 NAL 又是整个 Access Unit 的最后一个 NAL。

大 IDR 的中间 FU 即使包已满也不能设 Marker，只有 E=1 的最后 FU 且 AU 结束才设置。

```cpp
setTimestamp(framePresentationTime);
```

当前项目定制为每个输出 RTP 包都重写 timestamp，确保一个 AU 的 SPS/PPS/SEI/slice 或一个 NAL 的多个 FU 使用相同 timestamp。

如果上游为同一 AU 的不同 NAL 给出了不同 presentation time，这里仍会把各 RTP 包按各自传入值写入，真正一致性依赖 framer 的 AU 时间管理。

## 81. MJPEG source：为什么 RTP 不能直接发送完整 JPEG 文件

源码位置：`mjpeg_server_subsession.cpp`、`mpeg_frame_source.cpp`、`mjpeg_video_source.cpp`。

### 81.1 两层 source

```text
MJPEG_FRAME_SOURCE
    从 rtspFrameCall 取得完整 JPEG 字节
        ↓
MJPEG_Video_Source
    解析 JPEG marker，输出 scan data 和 JPEG RTP 元数据
        ↓
JPEGVideoRTPSink
```

RFC 2435 的 JPEG RTP payload 不是简单把 `.jpg` 文件连同 SOI/DQT/SOF/SOS/EOI 原样切片。RTP header 要单独描述 type、Q、宽高、restart interval，payload 主要发送 entropy-coded scan data，必要时携带量化表。

### 81.2 `MJPEG_FRAME_SOURCE` 取完整 JPEG

逻辑与 H264 source 相似：准备 `Fream_Info_t`，调用 `rtspFrameCall()`，获得 queue 中一个 MJPEG frame。

它把 `m_toDelay` 初始设为 100 微秒，空队列时快速轮询；成功后设为 -1。需要核对 scheduler 的 delay 参数类型，负数若转无符号可能形成异常长延时；如果被当成已到期则会立即执行。这里应改成明确的 0。

### 81.3 EOI 检查

```cpp
if (*(pFrameEnd - 1) != 0xFF || *pFrameEnd != 0xD9)
```

代码要求 frame 尾部是 `FF D9` EOI。没有 EOI 说明 VENC pack 不是完整 JPEG 或队列项被截断，不能安全解析。

指针 `pFrameEnd` 的定义必须确认是最后一个有效字节还是 one-past-end；若是标准 one-past-end，解引用会越界。排障时需结合前面赋值核对。

### 81.4 marker 扫描

解析器依次处理：

- SOI `FFD8`：图像开始；
- DQT `FFDB`：量化表；
- SOF0 `FFC0`：基线 JPEG、宽高和采样因子；
- DRI `FFDD`：restart interval；
- SOS `FFDA`：scan data 开始；
- EOI `FFD9`：图像结束。

JPEG segment length 是大端 16 位，包含长度字段自身。每次移动指针前都应验证 segment 未越过 frame end。

### 81.5 宽高为何除以 8 向上取整

```cpp
fWidth = (fWidthPixels + 7) / 8;
fHeight = (fHeightPixels + 7) / 8;
```

RFC 2435 JPEG RTP header 用 8 像素块为单位表达宽高，所以 1920 会变成 240。

源码对超过 2048 的尺寸直接把 width/height 清 0。这是旧 JPEG RTP 8-bit width/height 字段限制相关的保护，会限制更高分辨率 MJPEG 兼容性。

### 81.6 YUV sampling 决定 JPEG type

SOF component sampling factor：

- 常见 4:2:2 映射 type 0；
- 常见 4:2:0 映射 type 1。

客户端根据 type 恢复 MCU 组织。解析错误会表现为颜色错乱或无法解码，而不只是 SDP 问题。

### 81.7 只输出 scan data

找到 SOS 后跳过 SOS header，计算到 EOI 前的 `scanDataSize`，复制到 `fTo`。DQT、SOF、SOS 本身不作为普通 scan payload发送，其信息由 `JPEGVideoSource` getter 供 RTP sink 构造 JPEG payload header。

如果 scan data 大于 fMaxSize，会设置 `fNumTruncatedBytes` 并截断；这会使当前 JPEG 图像不可解码，但不会越界。

### 81.8 MJPEG SDP 硬编码

subsession aux SDP 固定：

```text
b=AS:4096
a=framerate:30
a=framesize:<pt> <width>-<height>
```

帧率和带宽没有从实际 `VideoConfig` 获取。配置改变后即使 RTSP reboot，仍返回硬编码 30/4096，这是需要修正的协议描述不一致。

## 82. AAC source：AudioSpecificConfig、帧时长和当前 profile 缺陷

源码位置：`aac_audio_source.cpp`。

### 82.1 采样率索引表

```cpp
static unsigned const samplingFrequencyTable[16] = {
    96000, 88200, 64000, 48000,
    44100, 32000, 24000, 22050,
    16000, 12000, 11025, 8000,
    7350, 0, 0, 0
};
```

它与 MPEG-4 AudioSpecificConfig/ADTS 的 frequencyIndex 定义一致。IPC 层传 index，source 反查实际 RTP clock rate。

### 82.2 `createNew()` 中构造的 ADTS header 没有被使用

代码生成 `adts_header[7]` 用于“验证”，但构造后既不检查也不输出。真正 RTSP 输入已经在 `CStreamAudio` 去掉 ADTS。

因此这几行对运行结果没有作用，可以删除或改成明确的参数一致性检查，避免读者误以为 source 会重新加 ADTS。

### 82.3 profile/object type 错位

```cpp
u_int8_t profile = 1; // 注释称 AAC LC
...
u_int8_t const audioObjectType = profile;
audioSpecificConfig[0] =
    (audioObjectType << 3) |
    (samplingFrequencyIndex >> 1);
```

在 ADTS 中 profile 字段 1 表示 AAC LC，因为 ADTS profile = objectType - 1；但 AudioSpecificConfig 要直接写 objectType，AAC LC 应为 2。

当前把 1 直接写入 ASC，会向客户端声明 AAC Main。如果海思 AENC 实际输出 AAC LC，SDP 与码流不一致，部分解码器可能容忍，严格客户端可能失败。

正确逻辑应明确变量语义：

```text
ADTS profile field = 1
AudioSpecificConfig audioObjectType = profile + 1 = 2
```

### 82.4 每帧 1024 samples

```cpp
fuSecsPerFrame = 1024 * 1000000 / fSamplingFrequency;
```

AAC-LC 常规 frame 是 1024 samples。16 kHz 时 64 ms；48 kHz 时约 21.333 ms。

如果使用 AAC-LD/ELD 或编码器配置不同，samples per frame 可能不是 1024，当前计算会错。

### 82.5 构造时就发送 START

```cpp
if (clientFun)
{
    status = RTSPCLIENT_START;
    clientFun(&status);
}
```

只要 SDP dummy audio source 被构造，就会把整个 `Live_Stream_Info_t::request` 设 1；析构又发 STOP。视频和音频 source 共用同一个 bool，二者的 START/STOP 会互相覆盖。

例如 video source 仍在使用，但 dummy audio source 析构发 STOP，可能短暂阻止视频入队。这进一步说明 request 应是引用计数或独立 video/audio consumer state。

### 82.6 `getNextFrame1()` 的时间戳先递增又被覆盖

代码先：

```cpp
if (first) gettimeofday(&fPresentationTime);
else fPresentationTime += fuSecsPerFrame;
```

随后马上：

```cpp
gettimeofday(&m_aacCurTime, NULL);
fPresentationTime = m_aacCurTime;
```

第二段无条件覆盖前面的 sample-based 累计，因此实际每帧使用当前墙钟，而不是严格连续的 1024-sample 时间轴。

网络/线程调度抖动会直接进入 AAC RTP timestamp，音画同步更不稳定。应选择一种模型：硬件 PTS，或首帧 wall-clock + samples 累计，而不是两者先后覆盖。

## 83. `MPEG4GenericRTPSink`：AAC AU Header 四个字节逐位解释

### 83.1 SDP fmtp

```text
streamtype=5
profile-level-id=1
mode=AAC-hbr
sizelength=13
indexlength=3
indexdeltalength=3
config=<ASC hex>
```

- streamtype 5 表示 audio stream；
- AAC-hbr 是高比特率 mode；
- AU-size 用 13 bit；
- AU-index 用 3 bit；
- config 是 2 字节 ASC 的十六进制。

### 83.2 special header size=4

```cpp
headers[0] = 0;
headers[1] = 16;
```

前 16 位 `AU-headers-length=16`，表示后面 AU Header Section 有 16 bit。

```cpp
headers[2] = fullFrameSize >> 5;
headers[3] = (fullFrameSize & 0x1F) << 3;
```

把完整 AAC AU 字节数放入 13-bit AU-size，高 8 位写 headers[2]，低 5 位放 headers[3] 高 5 位；剩余低 3 位是 AU-index，默认 0。

### 83.3 Marker

只有当前 packet 是该 AAC AU 的最后或唯一分片时设置 Marker。正常 AAC frame 很小，通常一个 RTP packet 包含一个 AU。

`frameCanAppearAfterPacketStart=False` 明确禁止一个 RTP 包聚合多个 AAC AU，简化 AU header。

## 84. `G711AudioStreamSource` 与 `SimpleRTPSink`

### 84.1 固定音频参数

```cpp
fNumChannels = 1;
fSamplingFrequency = 8000;
fBitsPerSample = 8;
```

无论上层 Audio_Source_Info 如何，source 固定为窄带 8 kHz 单声道。若产品配置允许其他采样率，SDP/subsession 与 source 会不一致。

### 84.2 注释说 20 ms，代码实际 40 ms

```cpp
unsigned desiredSamplesPerFrame =
    (unsigned)(0.04 * fSamplingFrequency);
```

0.04 秒是 40 ms，8 kHz 对应 320 samples/bytes。注释“close to 20 ms”与实现不一致。

不过 source 并不主动从一个大 PCM 流切 320 字节，它通过 callback 取得 queue 中整帧。`fMaxSize` 被收缩到 preferred size，但 callback 没有 capacity 参数，若 queue frame 超过 320 字节仍可能 memcpy 越界。

### 84.3 duration

```cpp
bytesPerSample = channels * bits / 8; // =1
duration = frameSize * 125us;
```

320 字节即 40 ms，RTP timestamp 增量 320。

### 84.4 静态 payload type

- PCMU/8000/1 -> PT 0；
- PCMA/8000/1 -> PT 8。

静态 PT 时 `RTPSink::rtpmapLine()` 返回空，因为客户端按 RTP/AVP profile 已知道 codec 和 clock。

非 8k/mono 时改用动态 PT，但 source 又固定 8k/mono，所以当前实际始终走静态 PT。

### 84.5 audio Marker

`SimpleRTPSink` 构造时 `doNormalMBitRule=True`，但实现只对非 audio 设置 `fSetMBitOnLastFrames`。因此普通 G.711 audio packet 不设置 Marker，符合连续音频常见用法；talkspurt 起点若需要 Marker，要外部显式 `setMBitOnNextPacket()`。

## 85. `G726AudioStreamSource`：位宽、帧长和轮询单位

### 85.1 参数

默认：

```text
1 channel, 8000 Hz, 4 bit/sample = 32 kbit/s
```

如果 `bitWidth` 在 2..5，分别支持 16/24/32/40 kbit/s。

### 85.2 preferred size

20 ms 时 samples=160：

```text
G726-32: 160 * 4 bit = 640 bit = 80 bytes
```

代码使用 `(+7)/8` 向上取整，支持非整字节结果。

### 85.3 duration

```cpp
samplesInFrame = frameSize * 8 / bitsPerSample;
duration = samplesInFrame * 125us;
```

80 字节、4 bit/sample 得到 160 samples、20 ms。

### 85.4 `m_toDelay=10` 是微秒

live555 `scheduleDelayedTask` 单位是微秒。G726 source 初始化/空队列设置 10，意味着 10 微秒轮询，而 H264/AAC/G711 使用 10000，即 10 ms。

空队列时 10 微秒会导致 event loop 高频调度，显著占 CPU。若作者意图 10 ms，应写 10000。

### 85.5 RTP MIME

subsession 根据位宽选择 `G726-16/24/32/40`，使用动态 payload type 和采样率 clock。

这里只描述码率，不描述 packing bit order。若客户端与海思编码器在 RFC 3551 packing/AAL2 packing 上不一致，会出现持续噪声。

## 86. 视频配置更新：为什么 RTSP 与 VENC 的重启顺序很关键

源码位置：`CStreamVideo::setVideoConfig()`。

### 86.1 先比较旧配置

```cpp
if (resolution changed)
{
    bIsSetVpss = true;
    bIsUpdateOsd = true;
}

if (codec || videoType || frameRate changed)
{
    bIsResetRtsp = true;
}
```

- 分辨率改变需要更新 VPSS 输出、wrap、VENC 和 OSD 坐标；
- codec 改变会替换 RTP sink 类型和 SDP fmtp；
- videoType 改变决定是否有 audio track；
- frameRate 改变影响 source duration/SDP，因此重建 RTSP。

码率/GOP 改变没有直接设置 RTSP reboot，因为 RTP payload format 不变；但 estimated bitrate/SDP b=AS 当前又不是从配置动态生成，所以即使 reboot 也不会准确反映码率。

### 86.2 先更新 RTSP 快照并 reboot

```cpp
m_configManager.updateVideoConfig(stVideoConfig);
CRtspServer::instance()->setVideoConfig(allConfigs);
if (bIsResetRtsp)
    CRtspServer::instance()->reboot();
```

这发生在停止旧 VENC 线程和 `streamVenc_reset()` 之前。

因此存在窗口：

```text
RTSP 已按新 codec 创建 session
VENC 仍短暂输出旧 codec
```

若客户端恰好连接，SDP 可能声明 H.265，但 queue 进入 H.264 pack。更稳妥的事务顺序是：

1. 暂停 RTSP 接入/生产；
2. 停止 VENC 取流并清队列；
3. 重建 VPSS/VENC；
4. 请求新 codec IDR/参数集；
5. 重建 RTSP session；
6. 重新允许客户端。

### 86.3 停线程再 reset VENC

```cpp
m_bVencFlag[nId].store(false);
join(vencThread);
vpssUnbindVencModule(nId);
...
streamVenc_reset(handle, newConfig, roiConfig);
vpssBindVencModule(nId);
m_bVencFlag[nId].store(true);
start thread;
```

这是正确的 handle 使用顺序：先停止使用者，再修改资源，再恢复生产。

失败路径需要注意：一旦线程已 join、VPSS 已 unbind，后续 VPSS crop/wrap/VENC reset 任一失败就直接 return，通道不会自动恢复旧配置，可能永久停流。

### 86.4 主动 IDR

VENC 重启和重新绑定后立即 `request_idr(nId)`，让参数集尽快进入 RTSP/RTMP。此时 RTSP 如果之前已 reboot 并有客户端请求，能缩短等待时间。

## 87. 音频配置更新：为什么 RTSP reboot 后还要 audio reboot

```cpp
if (oldFormat != newFormat)
    bIsReset = true;
```

只有编码格式变化触发完整重启；降噪、输入 track mode、输出设备、音量可在线设置。

格式变化时：

```cpp
CRtspServer::setAudioConfig(newConfig);
CRtspServer::reboot();
CStreamAudio::reboot();
```

同样是 RTSP 先按新音频格式创建 subsession，AENC 后重启到新格式，存在短暂协议/数据不一致窗口。

此外两个 reboot 都可能失败，但 RTSP reboot 返回值未检查，函数最终仍可能返回 OK。需要把更新做成可回滚事务并传播错误。

`m_bIsAac` 在重启前更新。AENC 旧格式仍运行的短窗口内，发送门控可能已经按新格式判断，导致帧被错误丢弃或送错处理分支。

## 88. 用户、Digest、端口配置函数逐句说明

### 88.1 `update_userInfo()`

```cpp
if (strUser.empty() || strPwd.empty())
    return ERR_PARAM_NULL;
m_strUser = strUser;
m_strPwd = strPwd;
if (bReboot) reboot();
```

- 禁止空账号/密码；
- 只更新内存成员，不直接修改当前 live555 authDB；
- 必须 reboot 才让新认证库生效；
- reboot 返回值被忽略，即使端口重建失败也返回 OK；
- string 参数按值传入产生复制，可改 const reference。

### 88.2 `updateRtspDigestAlgorithm()`

读取安全配置，如果枚举不同就更新成员。它自身不 reboot；调用方 `Network::SetAuthMethod` 在成功后显式 reboot。

这种“setter 只更新快照、调用方负责重启”的模式与 `setPort()` 不一致，后者内部自动 reboot。API 行为不统一容易造成漏重启或双重启。

### 88.3 `setPort()/setQosDscp()`

二者更新成员后立即 reboot。没有检查 reboot 返回值，始终返回 OK；且 RTSP disabled 时会被重新开启。

### 88.4 `updateNetworkConfig()`

它只重算展示 URL，不重新 bind socket。监听 socket 通常绑定通配地址，所以 IP 改变后无需重启 server；但现有客户端的 SDP/Content-Base 与网络可达性仍可能变化。

函数会记录带账号密码 URL，存在凭证日志泄露。

## 89. `BasicTaskScheduler`：live555 单线程事件循环每一步做什么

源码位置：`BasicTaskScheduler0.cpp`、`BasicTaskScheduler.cpp`。

### 89.1 delayed task

```cpp
if (microseconds < 0) microseconds = 0;
DelayInterval delay(sec, usec);
AlarmHandler* alarm = new AlarmHandler(proc, data, delay);
fDelayQueue.addEntry(alarm);
```

- 负 delay 被归零，因此 MJPEG source 的 -1 实际是立即任务，不会变成超长延迟；
- DelayQueue 按到期时间排序；
- TaskToken 是 alarm token，unschedule 时从队列移除并 delete。

### 89.2 `doEvent()`

```cpp
void doEvent() { SingleStep(); }
```

项目 event thread 外层自己 while 检查 atomic quit，每轮执行一个 SingleStep。

### 89.3 select timeout

`SingleStep()` 从 DelayQueue 取得最近 alarm 时间作为 select timeout。scheduler 默认 max granularity=10000 微秒，并通过周期 tick 确保即使没有 socket/其他 timer，最多约 10 ms 返回一次。

这也解释了：`triggerEvent()` 本身没有 pipe/eventfd 唤醒 select，但控制 destroy 通常仍会在 10 ms tick 内得到处理。

### 89.4 每轮只处理一个 socket handler

代码找到一个 ready handler 后调用并 break。`fLastHandledSocketNum` 记录上次位置，下轮从后面继续，避免一个持续可读 socket饿死其他 socket。

在 handler 前先设置 last socket，是为了 handler 内嵌套 `doEventLoop()` 时也能保持遍历进度。

### 89.5 event trigger 在 socket 后处理

```cpp
socket handler
 -> triggered event
 -> delayed alarm
```

trigger callback 可能修改 socket handler 集，因此放在当前 socket callback 后处理，避免遍历结构中途失效。

### 89.6 event trigger 的跨线程问题

```cpp
fTriggeredEventClientDatas[i] = clientData;
fTriggersAwaitingHandling |= eventTriggerId;
```

注释允许外部线程调用，但 bitmask 只是 `volatile`，clientData 数组也是普通指针，没有 atomic/mutex。按 C++ 内存模型仍是数据竞争。

而且同一个 trigger 在处理前被两次触发，第二次 clientData 会覆盖第一次。项目 destroy 是同步串行调用时通常不会碰到，但并发 reboot/destroy 需要外层严格串行化。

### 89.7 调试文件的性能影响

当前 `SingleStep()` 每轮调用两次 `access("/opt/cam/bin/rtsp_debug.txt")`。文件存在时每次 select 前后 printf；即使不存在，持续 filesystem access 也在事件循环热路径增加系统调用。

线上诊断开关更适合用 atomic flag、signal 或低频缓存，而不是每个 event step 查文件。

## 90. `Groupsock`：一个 RTP 包如何发往多个 UDP 客户端

源码位置：`groupsock/Groupsock.cpp`。

### 90.1 destination 链表

每个 `destRecord` 保存：

- 目标 IP；
- 目标端口；
- TTL；
- client session id；
- next 指针。

`StreamState::startPlaying()` 为每个 UDP 客户端调用 `addDestination()`，同一个 Groupsock 因此形成多个目标。

### 90.2 `removeAllDestinations()`

RTP sink 刚创建时，OnDemand 基类先移除构造 Groupsock 时的 dummy destination。真正 PLAY 后才按 client session id 添加目标。

### 90.3 `output()`

```cpp
for (destRecord* dest = fDests;
     dest != NULL;
     dest = dest->fNext)
{
    if (!write(destAddr, ttl, buffer, size))
    {
        writeSuccess = False;
        break;
    }
}
```

一份已经构造好的 RTP packet 对每个 UDP destination 调一次 sendto。不会为每个客户端重新编码或重新做 RTP packetization。

第一个 destination 发送失败后 break，后面的客户端本轮也收不到包。若一个失效目的地址持续使 write 返回错误，可能影响同组其他客户端；需要看 UDP error 行为和是否及时移除 destination。

发送成功后全局/实例 traffic stats 增加一次，而不是按 destination 数量增加，因此统计更接近“逻辑 RTP 包”而不是实际网卡发出的总副本字节。

### 90.4 UDP 与 TCP 的复制开销不同

- UDP：同一个 buffer 多次 sendto，内核为每个目的地复制；
- TCP：`RTPInterface` 遍历 stream record，写入不同 socket；
- 编码、NAL 解析和 RTP header 构造都只做一次。

这正是 `reuseFirstSource` 对多客户端节省 CPU 的主要来源。

## 91. 客户端数：全局 32、声明 4 与实际 session 引用的区别

### 91.1 TCP connection 全局上限 32

```cpp
std::lock_guard<std::mutex> lock(gs_connectMutex);
if (gs_nClientCount >= 32)
{
    closeSocket(clientSocket);
    return NULL;
}
++gs_nClientCount;
```

这是 listen accept 后最早的准入，统计的是 RTSP TCP connection，不是已 SETUP/PLAY 的媒体客户端。

同一客户端若建立多条 TCP 连接会占多个计数；RTSP-over-HTTP 还可能使用成对连接。

connection 关闭时 `closeSocketsRTSP()` 在同一 mutex 下减计数。

### 91.2 `MAX_CLIENT_NUM=4`

IPC 把它写进 `Rtsp_Create_Info_t::param1`，但前述设置顺序使 `rtsp_setclient_maxNum()` 首次查不到 session。

### 91.3 `ServerMediaSession::fReferenceMax`

构造默认设 4，但当前 `incrementReferenceCount()` 没有比较 fReferenceMax 后拒绝。也就是说它只是一个未被准入逻辑使用的字段。

### 91.4 `CLIENTMAX=4` 更像信息数组容量

```cpp
Connect_Client_Info_t fClientInfo[CLIENTMAX];
```

普通 `incrementReferenceCount()` 只增加 count，不填写 client info。另一个带 socket 的重载可以填 IP/socket，但当前 SETUP 路径使用无 socket 版本。

`decrementReferenceCount()` 在 count 为 1 时清 `fClientInfo[1]` 后再减到 0，实际第一客户端若存于 index 0，会出现 off-by-one 清理错误。

### 91.5 产品实际容量结论

当前源码能明确证明的是：

- 最多接受 32 条并发 RTSP TCP connection；
- `ServerMediaSession` 引用计数可超过 4；
- 4 路限制未形成有效拒绝逻辑；
- fClientInfo 只能安全记录前 4 项且当前未可靠填充。

因此目标板必须通过第 5、6 个客户端实测，不应把宏名当成行为证据。

## 92. 源码覆盖审计：本项目 RTSP 可执行路径已经讲到哪里

### 92.1 控制面覆盖

| 路径 | 对应详细章节 |
|---|---|
| listen、ClientConnection | 51、56、57 |
| RTSP 请求半包/粘包 | 57 |
| OPTIONS/GET_PARAMETER | 57、61 |
| Digest | 58 |
| DESCRIBE | 59 |
| SETUP、UDP/TCP transport | 60、66 |
| PLAY/PAUSE/TEARDOWN | 61、67、68 |
| session timeout/RTCP liveness | 57、73 |

### 92.2 SDP 与媒体对象覆盖

| 路径 | 对应详细章节 |
|---|---|
| ServerMediaSession/track | 62 |
| session SDP | 63 |
| dummy source/sink | 64 |
| `m=`, `a=rtpmap`, `a=fmtp` | 65 |
| source/sink/groupsock 创建 | 66 |
| 多客户端共享 | 67、68、91 |

### 92.3 RTP/RTCP/网络覆盖

| 路径 | 对应详细章节 |
|---|---|
| RTP seq/SSRC/timestamp | 69 |
| RTP header build | 70 |
| async pull、overflow、pacing | 71、72 |
| UDP/TCP interleaved write | 72、90 |
| RTCP SR/RR/SDES/BYE | 73 |
| scheduler/select/event trigger | 89 |

### 92.4 codec 覆盖

| codec | 对应详细章节 |
|---|---|
| H.264/H.265 subsession/source | 74-76 |
| Annex-B、NAL、Access Unit | 77 |
| H.264/H.265 SDP fmtp | 78 |
| FU-A/H.265 FU、Marker | 79-80 |
| MJPEG/JPEG RTP | 81 |
| AAC source/ASC/AU Header | 82-83 |
| G.711 | 84 |
| G.726 | 85 |

### 92.5 IPC 上层覆盖

| 路径 | 对应详细章节 |
|---|---|
| 初始化/退出 | 34、39 |
| VENC pack 生命周期 | 35-36 |
| 主子码流路由 | 37-38 |
| IDR callback | 40、46、48 |
| AAC 去 ADTS | 41 |
| frame queue | 42 |
| RTSP init/session | 43-44 |
| push/pop callback | 45-47 |
| deinit/reboot | 49 |
| 视频/音频动态配置 | 86-88 |

### 92.6 不属于当前产品 RTSP Server 主路径的 live555 目录

以下源码在仓库中存在，但当前 IPC server 初始化链没有使用：

- `testProgs` 示例播放器/streamer；
- `hlsProxy`；
- `ProxyServerMediaSession`；
- RTSP client 封装；
- SIP client；
- 文件型 MPEG/Matroska/Ogg server subsession；
- RTSP REGISTER/DEREGISTER 业务；
- multicast/SSM 产品配置；
- SRTP、RTSPS、RTSP-over-HTTP tunnel 的启用入口。

本指南对它们在通用分派中的存在做了说明，但没有逐函数展开，因为它们不在 `stream` 进程创建 `/Streaming/Channels/101/102` 的可执行数据路径上。接手当前 IPC RTSP 问题时，应优先以上述覆盖矩阵为准。

## 93. 源码审读后的修复优先顺序

### 第一批：先消除未定义行为

1. `VideoFrame_S/AudioFrame_S` 统一 malloc/free 或改 RAII。
2. `Live_Stream_Info_t`、`Rtsp_Server_Info_t` 不再用 calloc/malloc 绕过 C++ 成员构造。
3. event loop 退出使用 wakeup + join 后再释放 scheduler/environment。
4. callback 增加目标 capacity，所有 memcpy 做边界检查。

### 第二批：建立正确跨线程状态

1. request/requestIFrame 改 atomic 或锁保护。
2. request 从 bool 改为 video/audio/source/client 引用计数状态。
3. deinit 与 send 使用共享生命周期锁或先停生产线程。
4. BasicTaskScheduler event trigger 使用 atomic/wakeup fd 或外层串行保证。

### 第三批：修复协议一致性

1. AAC LC AudioSpecificConfig object type 修正并抓包验证。
2. VENC PTS 传到 live555，统一单调媒体时钟。
3. H.265 presentation time 与 H.264 对齐。
4. 修复 H264/MJPEG estimated bitrate 单位。
5. MJPEG SDP 帧率/带宽读取真实配置。
6. codec/audio 更新按事务顺序重启，避免新 SDP 配旧数据。

### 第四批：低延迟与可观测性

1. 队列从 drop-new 改成 codec-aware 实时策略。
2. G726 空队列轮询从 10 微秒修正为合理事件/延时。
3. 暴露 RTCP RR、队列水位、IDR 延时和 drop 原因。
4. 减少热路径 heap allocation 和 filesystem access。

## 94. 本文的验证边界

本文逐函数说明以当前工作区源码为权威，并完成了函数名、代码块、章节和 Markdown 结构核对。

但是设备运行链接的是预编译 `libRtspServer.so`。除非重新构建该库并在目标板核对实际加载文件，否则：

- live555 源码中看到的定制不能自动视为目标固件已包含；
- 主程序中的 `rtsp_server.cpp` 行为可随主程序重编生效；
- `.so` 内部 RTSP/RTP/RTCP 行为必须用 build id/hash、日志或抓包再次证明。

因此本文给出的风险分两类：

```text
主程序源码直接确认的问题
    例如 malloc/delete、calloc unique_ptr、queue 策略

live555 源码镜像确认的问题
    例如 event loop 未 join、AAC ASC、scheduler trigger
    需要确认当前 libRtspServer.so 与源码版本一致
```

这一区分应贯穿后续所有修复和回归验证。
