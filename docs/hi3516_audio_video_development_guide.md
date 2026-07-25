# Hi3516 IPC 音视频开发详细文档

> 目标：这份文档面向“快速熟悉项目并应付面试”。重点不是泛泛解释音视频概念，而是结合当前工程源码，把本项目的采集、处理、编码、缓存、推流、协议和 SDK 配置边界讲清楚。  
> 建议先按“面试速记版”背链路，再回到源码位置逐段阅读。

## 1. 面试速记版

本项目在 Hi3516 IPC 上的核心音视频链路可以概括为：

```text
视频:
Sensor -> ISP -> VI -> VPSS -> VENC -> 编码帧分发
                               |       |-> RTSP/RTMP 预览
                               |       |-> GB28181 RTP/PS
                               |       |-> 录像
                               |       |-> JPEG 抓图
                               |
                               |-> VPSS AI 通道 -> AI 算法/抓拍/检测

音频:
AI(麦克风/LineIn) -> 音量处理/VQE/AEC -> AENC
                                      |-> AAC 裸流 -> RTSP/录像
                                      |-> G.711 重采样编码 -> RTSP
                                      |-> algo_send_audioStreamData -> 音频算法

SDK/TVSDK:
SDK Client -> NET_TV_* 命令 -> IPC tvsdk 回调 -> ActionCode -> CTaskManage -> 业务模块
```

面试时可以这样讲：

```text
项目里 CStreamVideo 是视频链路总控，启动时先初始化 MPP 系统和 VB 共享池，再初始化 VI、ISP、VPSS、VENC。
VI 从 sensor 采集 2880x1620 Bayer 数据，VPSS 负责缩放、低延迟、AI 分支和 Wrap，VENC 负责 H.264/H.265/JPEG 编码。
编码线程从 VENC 取码流后解析 NAL 类型，主码流分发到 RTSP、GB28181 和录像，子码流主要用于 RTSP 和录像，AI 通道同时用于 JPEG 抓图。
RTSP 侧使用线程安全有界队列，队列满时主动丢帧，新客户端接入时请求 IDR 并优先发送 I 帧，保证实时性。
音频侧 AI 固定按 16k/16bit 采集，AENC 固定有 AAC 通道；如果配置 G.711，会在 AI 线程里做 16k 到 8k 的重采样再编码推流。
```

## 2. 建议阅读顺序

第一轮只看主链路：

1. `Hi3516/hi3516_ipc/main_app/stream_main.cpp:61`  
   `initModules()`，看模块启动顺序。
2. `Hi3516/hi3516_ipc/main_app/stream_media/video/stream_video.cpp:44`  
   `CStreamVideo::init()`，看视频初始化总控。
3. `Hi3516/hi3516_ipc/main_app/stream_media/video/sys/stream_sys.cpp:148`  
   `streamSys_init()`，看 MPP 系统和 VB 共享池。
4. `Hi3516/hi3516_ipc/main_app/stream_media/video/vi/stream_vi.cpp:13`  
   `streamVi_init()`，看 VI/sensor 输入。
5. `Hi3516/hi3516_ipc/main_app/stream_media/video/vo/stream_vpss.cpp:16`  
   `streamVpss_init()`，看 VPSS 通道设计。
6. `Hi3516/hi3516_ipc/main_app/stream_media/video/vo/stream_venc.cpp:68`  
   `streamVenc_init()`，看 VENC 编码参数。
7. `Hi3516/hi3516_ipc/main_app/stream_media/video/stream_video.cpp:785`  
   `get_vencStream()`，看编码取流线程。
8. `Hi3516/hi3516_ipc/main_app/stream_media/video/venc_channel_handler.cpp:31`  
   `CMainChannelHandler::handleFrame()`，看编码帧分发。
9. `Hi3516/share/ipc_share/push_stream/rtsp/rtsp_server.cpp:430`  
   `CRtspServer::sendVideoData()`，看 RTSP 入队和丢帧。
10. `Hi3516/share/ipc_share/push_stream/rtsp/rtsp_server.cpp:18`  
    `rtspFrameCall()`，看 RTSP 取帧和 I 帧优先。

第二轮补音频：

1. `Hi3516/hi3516_ipc/main_app/stream_media/audio/stream_audio.cpp:109`
2. `Hi3516/hi3516_ipc/main_app/stream_media/audio/ai/stream_ai.cpp:14`
3. `Hi3516/hi3516_ipc/main_app/stream_media/audio/codec/stream_aenc.cpp:13`
4. `Hi3516/hi3516_ipc/main_app/stream_media/audio/stream_audio.cpp:857`
5. `Hi3516/hi3516_ipc/main_app/stream_media/audio/stream_audio.cpp:1035`

第三轮补协议和 SDK 边界：

1. `Hi3516/share/ipc_share/protocols/gb28181/sip/RtpServer.cpp:69`
2. `Hi3516/share/ipc_share/protocols/gb28181/sip/media/MediaRtp.cpp:487`
3. `Hi3516/share/ipc_share/protocols/gb28181/sip/media/MediaPs.cpp:1002`
4. `Hi3516/share/ipc_share/protocols/tvsdk/src/callbacks/tvsdk_callbacks.cpp:635`
5. `Hi3516/share/ipc_share/protocols/tvsdk/src/convert/tvsdk_convert.cpp:397`

## 3. 程序启动和生命周期

### 3.1 总启动入口

入口文件：

```text
Hi3516/hi3516_ipc/main_app/stream_main.cpp
```

关键位置：

- `stream_main.cpp:61`：`initModules()`
- `stream_main.cpp:191`：`deinitModules()`
- `stream_main.cpp:247`：`main()`

启动顺序是：

```text
CConfigManager
-> CCryptoInit
-> CStreamVideo
-> CStreamAudio
-> CPushStream
-> ControlManage
```

这套顺序是合理的：

- 配置必须先加载，因为视频、音频、RTSP、SDK 都依赖配置。
- 视频先于推流初始化，因为 RTSP/GB28181 需要知道码流类型、分辨率、帧率。
- 音频先于推流初始化，因为 RTSP 初始化时要根据音频格式设置 `nAudioType`。
- 控制模块最后启动，因为控制接口可能会读写音视频配置，必须等业务模块基本可用。

### 3.2 视频模块生命周期

入口：

```text
Hi3516/hi3516_ipc/main_app/stream_media/video/stream_video.cpp:44
```

`CStreamVideo::init()` 的核心流程：

```text
1. 初始化主/子/JPEG 通道处理策略
2. 初始化 H264/H265/SVAC3/MJPEG NAL 解析策略
3. 注册配置回调和 IDR 请求回调
4. 从配置管理器读取视频配置
5. streamSys_init() 初始化 MPP SYS 和 VB 共享池
6. streamVi_init() 初始化 VI
7. CIspControl::init() 初始化 ISP
8. streamVpss_init() 初始化 VPSS
9. 根据 AI 通道尺寸构造 JPEG 编码配置
10. streamVenc_init() 初始化主码流、子码流、JPEG VENC
11. 为每个 VENC 通道启动 get_vencStream 取流线程
12. algo_detect_init() 初始化 AI
13. 启动 get_vpssStream 线程向 AI 送 VPSS 原始帧
14. COsdManage::init() 初始化 OSD
15. bindModule() 绑定 VI->VPSS、VPSS->VENC
```

关键源码：

- `stream_video.cpp:66`：`streamSys_init(videoConfigs)`
- `stream_video.cpp:73`：`streamVi_init()`
- `stream_video.cpp:84`：`streamVpss_init()`
- `stream_video.cpp:102`：`streamVenc_init()`
- `stream_video.cpp:112`：启动 VENC 取流线程
- `stream_video.cpp:125`：启动 VPSS AI 取帧线程
- `stream_video.cpp:630`：`bindModule()`

绑定关系：

```cpp
mppVi_bind_vpss(VI dev, VI pipe, VPSS_MAIN_SUB, 0);
mppVpss_bind_venc(VPSS_MAIN_SUB, VPSS_CHANNEL_MAIN, VENC_CHN_MAIN);
mppVpss_bind_venc(VPSS_MAIN_SUB, VPSS_CHANNEL_SUB,  VENC_CHN_SUB);
mppVpss_bind_venc(VPSS_MAIN_SUB, VPSS_CHANNEL_AI,   VENC_CHN_JPEG);
```

工程意义：

- 主码流和子码流由 VPSS 不同通道输出给不同 VENC 通道。
- AI 通道不只给算法用，也绑定到了 JPEG VENC，用于抓拍编码。
- 绑定后数据在硬件模块之间流转，CPU 主要负责控制和取编码后的码流。

## 4. 视频参数和配置

### 4.1 默认主码流参数

源码：

```text
Hi3516/hi3516_ipc/main_app/stream_media/video/stream_video_config.cpp:122
```

`fill_default_videoConfig()` 中主码流配置：

| 参数 | 当前默认值 | 说明 |
|---|---:|---|
| 通道 ID | `0` | 主码流 |
| 视频类型 | `COMPOSITE_STREAM` | 复合流，RTSP 可以带音频 |
| 分辨率 | `2880x1620` | `PIXEL_WIDTH_2_5K` / `PIXEL_HEIGHT_2_5K` |
| 码控 | `CBR` | 固定码率 |
| 图像质量 | `MEDIUM` | 中等质量 |
| 帧率 | `30fps` | `FRAME_RATE_30` |
| 码率上限 | `4096 kbps` | 后续 VENC 会乘 0.75 |
| 平均码率 | `2048 kbps` | 传给 VENC 扩展参数 |
| 编码格式 | `H.265` | 默认 H265 |
| 智能编码 | `false` | 关闭 SmartP |
| Profile/复杂度 | `Main` | H.264 时映射 profile，H.265 保留复杂度字段 |
| GOP/I 帧间隔 | `50` | 30fps 下约 1.67s 一个 I 帧 |
| SVC | `Disable` | 默认关闭 |
| 码流平滑 | `50` | 清晰和平滑折中 |

### 4.2 默认子码流参数

同一函数中子码流配置：

| 参数 | 当前默认值 | 说明 |
|---|---:|---|
| 通道 ID | `1` | 子码流 |
| 分辨率 | `704x576` | D1 |
| 码控 | `CBR` | 固定码率 |
| 帧率 | `30fps` | 与主码流一致 |
| 码率上限 | `1024 kbps` | 后续 VENC 会乘 0.75 |
| 平均码率 | `512 kbps` | 子码流平均码率 |
| 编码格式 | `H.265` | 默认 H265 |
| GOP | `50` | 和主码流一致 |

### 4.3 JPEG 抓图通道

在 `CStreamVideo::init()` 中，JPEG 通道不是单独从配置文件读取，而是基于子码流配置改出来：

源码：

```text
Hi3516/hi3516_ipc/main_app/stream_media/video/stream_video.cpp:88
```

设计：

- `nId = VENC_CHN_JPEG`
- `enVideoCodec = JPEG`
- 分辨率使用 VPSS AI 通道宽高
- 帧率设置为 `FRAME_RATE_2`

也就是 JPEG 抓拍实际使用 AI 通道输出尺寸，而不是主码流 2.5K 原尺寸。这是为了降低抓拍编码和 AI 处理压力。

### 4.4 能力集配置

源码：

```text
Hi3516/hi3516_ipc/main_app/stream_media/video/stream_video_config.cpp:214
```

主码流能力：

- 支持 H.264、H.265、MJPEG。
- 支持分辨率：
  - `2880x1620`
  - `2560x1440`
  - `1920x1080`

子码流能力：

- 支持 H.264、H.265。
- 支持分辨率：
  - `704x576`
  - `640x480`
  - `352x288`

相关宏：

```text
Hi3516/share/ipc_share/common/define/video_define.h:26
Hi3516/share/ipc_share/common/define/video_define.h:45
Hi3516/share/ipc_share/common/define/video_define.h:39
Hi3516/share/ipc_share/common/define/video_define.h:60
```

其中：

```cpp
PIXEL_WIDTH_2_5K  = 2880
PIXEL_HEIGHT_2_5K = 1620
PIXEL_WIDTH_704   = 704
PIXEL_HEIGHT_576  = 576
```

### 4.5 配置结构体和 JSON 转换

配置结构：

```text
Hi3516/share/ipc_share/common/define/video_define.h:289
```

`VideoConfig_S` 关键字段：

- `nId`：码流 ID。
- `enVideoType`：视频流/复合流。
- `stVideoResolution`：分辨率。
- `enBitrateType`：CBR/VBR 等码控类型。
- `enFrameRate`：帧率枚举。
- `nBitrateUpperLimit`：码率上限。
- `nAverageBitrate`：平均码率。
- `enVideoCodec`：H264/H265/JPEG/MJPEG/SVAC3。
- `bSmartEnable`：智能编码开关。
- `nIFrameInterval`：I 帧间隔/GOP。
- `enSvcEnable`：SVC 开关。

JSON 转换：

```text
Hi3516/share/ipc_share/common/convert/video_convert.cpp:14
```

这里负责把配置文件/接口 JSON 与 `VideoConfig_S` 互转。面试时如果被问“SDK 设置编码参数后怎么生效”，要从 SDK 回调走到 `VideoConfig_S`，再由配置回调触发视频模块重配。

## 5. MPP 系统和 VB 共享池

### 5.1 MPP SYS 初始化

源码：

```text
Hi3516/hi3516_ipc/main_app/stream_media/video/sys/stream_sys.cpp:148
```

`streamSys_init()` 做了这些事：

```text
1. mppSys_uninit()
2. mppVb_uninit()
3. 构造 ot_vb_cfg
4. 计算 VPSS-VENC Wrap buffer 大小
5. 设置 common_pool[0].blk_size / blk_cnt / remap_mode
6. mppVb_set_cfg()
7. mppVb_init()
8. mppSys_init()
```

这里的设计意图是：启动前先清理旧状态，再重新配置 VB 池，避免上一次异常退出导致 MPP/VB 残留。

### 5.2 VPSS-VENC Wrap 缓冲

源码：

```text
Hi3516/hi3516_ipc/main_app/stream_media/video/sys/stream_sys.cpp:49
```

`streamSys_compute_vpssVencWrap()` 的关键配置：

```cpp
stWrapParam.frame_rate = 30;
stWrapParam.all_online = TD_TRUE;
stWrapParam.large_stream_size.width  = PIXEL_WIDTH_2_5K;
stWrapParam.large_stream_size.height = PIXEL_HEIGHT_2_5K;
stWrapParam.small_stream_size.width  = PIXEL_WIDTH_1680;
stWrapParam.small_stream_size.height = PIXEL_HEIGHT_954;
```

关键点：

- Wrap 是 VPSS 到 VENC 的卷绕缓冲，目的是减少大分辨率下的内存占用和搬运成本。
- `large_stream_size` 使用主码流 2.5K。
- `small_stream_size` 被手动放大到 `1680x954`。源码注释说明这是为了避免业务压力过大时出现 `miss start`、`ring back`、`ring buf full` 等中断丢帧问题。
- 根据 sensor 类型设置 `full_lines_std`，例如 SC500AI/SC533HAI 5M sensor 走 `1700`。
- 最终调用：

```cpp
ss_mpi_sys_get_vpss_venc_wrap_buf_line(&stWrapParam, &u32BufLine);
ot_comm_get_vpss_venc_wrap_buf_size(&stBufAttr, u32BufLine);
```

### 5.3 common VB 池配置

在 `streamSys_init()` 中：

```cpp
stVbCfg.common_pool[0].blk_size = wrap_buf_size;
stVbCfg.common_pool[0].blk_cnt = 1;
stVbCfg.common_pool[0].remap_mode = OT_VB_REMAP_MODE_NONE;
```

面试解释：

```text
这里不是为每一帧都 malloc/free 软件内存，而是使用海思 MPP 的 VB 公共池承载硬件视频帧。
VPSS/VENC Wrap 模式下只配置一个较大的 block，通过环形方式复用，减少 DDR 压力和延迟。
```

需要注意：

- `blk_cnt = 1` 对延迟友好，但对突发阻塞更敏感。
- 所以项目在 Wrap 计算时主动放大小码流尺寸，换取更稳的 ring buffer 空间。

## 6. VI 视频采集

源码：

```text
Hi3516/hi3516_ipc/main_app/stream_media/video/vi/stream_vi.cpp:13
```

关键配置：

```cpp
stNeedParam.nDevId = 0;
stNeedParam.nChannel = 0;
stNeedParam.nWidth = PIXEL_WIDTH_2_5K;   // 2880
stNeedParam.nHeight = PIXEL_HEIGHT_2_5K; // 1620
stNeedParam.enPixelFormat = OT_PIXEL_FORMAT_RGB_BAYER_12BPP;
stNeedParam.enCompressMode = OT_COMPRESS_MODE_NONE;
memcpy(stNeedParam.aEnityName, "/dev/video11", sizeof("/dev/video11"));
```

sensor 通过宏选择：

```cpp
#ifdef SENSOR_SC500AI
    stNeedParam.sns_type = SC500AI_MIPI_5M_30FPS_10BIT;
#elif defined(SENSOR_SC533HAI)
    stNeedParam.sns_type = SC533HAI_MIPI_5M_30FPS_10BIT;
#endif
```

结合你的构建参数：

```text
设备型号: TV-3852T
镜头/sensor: sc533hai-f4mm
项目类型: itc
```

如果编译宏正确，VI 会选择 `SC533HAI_MIPI_5M_30FPS_10BIT`。

面试解释：

```text
VI 层主要负责从 sensor 节点采集原始 Bayer 数据，项目里输入尺寸是 2880x1620，像素格式是 12bit Bayer。
后续 ISP 会做去马赛克、降噪、曝光等图像处理，再进入 VPSS 转成 YUV420 半平面格式给编码和 AI 使用。
```

## 7. VPSS 视频处理

源码：

```text
Hi3516/hi3516_ipc/main_app/stream_media/video/vo/stream_vpss.cpp:16
```

### 7.1 VPSS Group

项目里 `streamVpss_init()` 按 `VPSS_GROUP_SUM` 创建 VPSS group。当前核心 group 是 `VPSS_MAIN_SUB`。

Group 属性：

```cpp
stVpssNeedParam.stVpssGrpAttr.enGrpComMode = COMPRESSMODE;
stVpssNeedParam.stVpssGrpAttr.enGrpPixelFormat = OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
stVpssNeedParam.stVpssGrpAttr.nMaxW = PIXEL_WIDTH_2_5K;
stVpssNeedParam.stVpssGrpAttr.nMaxH = PIXEL_HEIGHT_2_5K;
stVpssNeedParam.stVpssGrpAttr.nSrcFrameRate = -1;
stVpssNeedParam.stVpssGrpAttr.nDstFrameRate = -1;
```

`-1` 表示不在 group 层做固定帧率限制，由通道或后续模块控制。

### 7.2 VPSS 主通道

源码区域：

```text
stream_vpss.cpp:57
```

主通道配置：

```cpp
pVpssChnAttr->nWidth = 主码流宽;
pVpssChnAttr->nHeight = 主码流高;
pVpssChnAttr->nMaxWidth = 2880;
pVpssChnAttr->nMaxHeight = 1620;
pVpssChnAttr->bWrapEnable = TD_TRUE;
pVpssChnAttr->enChnComMode = OT_COMPRESS_MODE_SEG_COMPACT;
pVpssChnAttr->nSrcFrameRate = 30;
pVpssChnAttr->nDstFrameRate = 30;
pVpssChnAttr->bSmallStreamSize = TD_TRUE;
pVpssChnAttr->nSmallStreamWidth = 1680;
pVpssChnAttr->nSmallStreamHeight = 954;
```

设计解释：

- 主码流分辨率高，最吃带宽和 DDR。
- 开启 Wrap 和 `SEG_COMPACT` 压缩模式，减少内存访问压力。
- `bSmallStreamSize` 配合 Wrap 计算，用更保守的 buffer 尺寸提高稳定性。

### 7.3 VPSS 子通道

源码区域：

```text
stream_vpss.cpp:72
```

子通道配置：

```cpp
pVpssChnAttr->nWidth = 子码流宽;
pVpssChnAttr->nHeight = 子码流高;
pVpssChnAttr->nMaxWidth = 704;
pVpssChnAttr->nMaxHeight = 576;
pVpssChnAttr->bLowDelay = TD_TRUE;
```

设计解释：

- 子码流通常用于实时预览，分辨率低。
- 开启低延迟模式更适合预览场景。

### 7.4 VPSS AI 通道

源码区域：

```text
stream_vpss.cpp:79
```

AI 通道配置：

```cpp
pVpssChnAttr->nWidth = 1024;
pVpssChnAttr->nHeight = 576;
pVpssChnAttr->nMaxWidth = 1024;
pVpssChnAttr->nMaxHeight = 576;
pVpssChnAttr->nDepth = 6;   // 非人脸比对路径
pVpssChnAttr->nSrcFrameRate = 30;
pVpssChnAttr->nDstFrameRate = 30;
```

如果 `CAP_AI_FACE_COMPARE` 打开：

```cpp
nDepth = 2;
nDstFrameRate = 2;
```

设计解释：

- AI 不直接吃 2880x1620 的大图，而是由 VPSS 缩放到 1024x576，降低 NPU/CPU 负载。
- 人脸比对场景帧率降到 2fps，是典型的降载策略。
- `nDepth` 决定 VPSS 通道缓存深度，AI 处理慢时深度太小容易取帧失败，太大又增加延迟。

## 8. VENC 视频编码

源码：

```text
Hi3516/hi3516_ipc/main_app/stream_media/video/vo/stream_venc.cpp:68
```

### 8.1 基础编码参数

`streamVenc_init()` 里将 `VideoConfig_S` 转成 `HiVencNeedParam_S`：

```cpp
stVencNeedParam.unWidth = stVideoConfig.stVideoResolution.nWidth;
stVencNeedParam.unHeight = stVideoConfig.stVideoResolution.nHeight;
stVencNeedParam.unVirWidth = stVideoConfig.stVideoResolution.nWidth;
stVencNeedParam.unVirHeight = stVideoConfig.stVideoResolution.nHeight;
stVencNeedParam.enPixFormat = OT_PIXEL_FORMAT_YUV_SEMIPLANAR_420;
stVencNeedParam.nGop = stVideoConfig.nIFrameInterval;
stVencNeedParam.enGopMode = OT_VENC_GOP_MODE_NORMAL_P;
stVencNeedParam.nInFrameRate = 30;
stVencNeedParam.nOutFrameRate = stVideoConfig.getFrameRateAsInt();
```

关键点：

- 输入帧率固定按 30fps。
- 输出帧率按配置转换。
- GOP 直接使用 `nIFrameInterval`。
- 像素格式是 YUV420 半平面，和 VPSS 输出匹配。

### 8.2 编码格式映射

源码：

```text
stream_venc.cpp:103
```

映射关系：

| IPC 配置 | 海思 Payload |
|---|---|
| `H264` | `OT_PT_H264` |
| `H265` | `OT_PT_H265` |
| `SVAC3` | `OT_PT_SVAC3` |
| `JPEG` | `OT_PT_JPEG` |
| `MJPEG` | `OT_PT_MJPEG` |

### 8.3 码控模式

H.264：

```cpp
CBR -> OT_VENC_RC_MODE_H264_CBR
VBR -> OT_VENC_RC_MODE_H264_VBR
```

H.265：

```cpp
CBR -> OT_VENC_RC_MODE_H265_CBR
VBR -> OT_VENC_RC_MODE_H265_VBR
```

SVAC3/MJPEG 也做了对应 CBR/VBR 映射。

注意：

- 代码里有 CVBR/AVBR 注释，但当前实际启用的是 CBR/VBR。
- 如果面试官问 AVBR，回答时不要说项目已经用了 AVBR，应说“代码保留了 AVBR/CVBR 切换点，但当前默认和实际映射主要是 CBR/VBR”。

### 8.4 Profile / 编码复杂度

H.264 下：

```cpp
Baseline -> profile 0
Main     -> profile 1
High     -> profile 2
```

H.265 没有在这段代码里显式设置 profile，主要通过编码 payload 和码控配置走默认能力。

### 8.5 SmartP / 智能编码

源码：

```text
stream_venc.cpp:123
```

```cpp
if (stVideoConfig.bSmartEnable == true)
{
    stVencNeedParam.enGopMode = OT_VENC_GOP_MODE_SMART_P;
}
```

解释：

- 普通 GOP 是 `NORMAL_P`。
- 开启智能编码后切到 `SMART_P`，通常用于静态场景降低码率。
- 代价是 GOP 结构和码率波动更依赖芯片编码器策略，调试时要关注兼容性和首帧/I 帧。

### 8.6 主码流 Wrap

源码：

```text
stream_venc.cpp:130
```

```cpp
if (stVideoConfig.nId == VENC_CHN_MAIN)
{
    stVencNeedParam.bWrapEnable = TD_TRUE;
}
```

只有主码流 VENC 开启 Wrap。原因是主码流分辨率最大，收益最高。

### 8.7 码率处理

源码：

```text
stream_venc.cpp:197
```

```cpp
pHandle->stExParam.nBitRate = stVideoConfig.nBitrateUpperLimit * 0.75;
pHandle->stExParam.nMaxBitRate = pHandle->stExParam.nBitRate;
pHandle->stExParam.nMinBitRate =
    pHandle->stExParam.nBitRate - 2000 > 256 ?
    pHandle->stExParam.nBitRate - 2000 : 256;
pHandle->stExParam.nAverageBitrate = stVideoConfig.nAverageBitrate;
pHandle->stExParam.nBitrateSmoothing = stVideoConfig.nBitrateSmoothing;
```

设计解释：

- UI/SDK 配置的上限不直接等于编码器目标码率，而是乘 0.75 做保守处理。
- 最大码率等于目标码率。
- 最小码率至少 256kbps。
- 平均码率和平滑度保留给编码扩展参数。

面试可讲：

```text
这里做了一个工程上的降码率保护，避免按用户配置的上限直接打满编码器和网络带宽。
但这也意味着 SDK 设置 4096kbps 时，实际编码目标可能只有 3072kbps 左右，需要在联调时和平台说明清楚。
```

### 8.8 ROI 编码

源码：

```text
Hi3516/hi3516_ipc/main_app/stream_media/video/vo/stream_venc.cpp:26
```

核心处理：

- 校验 ROI 矩形是否有效。
- 将插件坐标系转换成真实视频分辨率坐标。
- 按 16 像素对齐。
- QP 默认：

```cpp
ROI_QP_DEFAULT = -8
ROI_QP_DEFAULT_FACTOR = -2
qp = -8 + level * -2
```

解释：

- ROI 通过降低 QP 提高感兴趣区域质量。
- 坐标转换和 16 对齐非常关键，否则硬编码器可能拒绝配置或出现区域错位。

## 9. 编码取流和帧分发

### 9.1 VENC 取流线程

源码：

```text
Hi3516/hi3516_ipc/main_app/stream_media/video/stream_video.cpp:785
```

每个 VENC 通道一个线程：

```cpp
m_getVencThread[i] = std::thread(&CStreamVideo::get_vencStream, this, i);
```

线程逻辑：

```text
while m_bVencFlag[channel]:
    mppVenc_get_stream(timeout=500ms)
    遍历 pack
    取 addr + offset / len - offset
    createFrame()
    channelHandler->handleFrame()
    freeFrame()
    mppVenc_release_stream()
    usleep(1000)
```

重点：

- `mppVenc_get_stream()` 拿到的是硬编码后的码流。
- 遍历 `pack_cnt` 是因为编码器一帧可能由多个 pack 组成。
- 处理完必须 `mppVenc_release_stream()`，否则编码器内部缓存会被占满。

### 9.2 NAL 类型解析

源码：

```text
stream_video.cpp:736
```

`createFrame()` 会根据编码类型选择 NAL parser：

```cpp
m_nalParsers[H264] = CH264NalParser
m_nalParsers[H265] = CH265NalParser
m_nalParsers[SVAC3] = CSvac3NalParser
m_nalParsers[MJPEG] = CMjpegParser
```

这个设计是策略模式：

- H264/H265/SVAC3/MJPEG 的 NAL 头格式不同。
- 统一产出 `VideoFrame_S::eType`，后续 RTSP 判断 I 帧、录像请求 IDR 都依赖它。

### 9.3 主码流分发

源码：

```text
Hi3516/hi3516_ipc/main_app/stream_media/video/venc_channel_handler.cpp:31
```

主码流分发：

```cpp
CPushStream::instance()->sendVideoData(pVideoFrame, true, true); // RTSP/RTMP
SIP::CRtpServer::instance()->sendVideoData(pVideoFrame);         // GB28181
CStreamServer::instance()->sendVideoData(pVideoFrame);           // 录像
```

注意：

- SVAC3 不送 RTSP。
- GB28181 只在主码流处理器里送。
- 录像是否使用主码流取决于 `CAP_RECORD_USE_MAIN_STREAM`。

### 9.4 子码流分发

源码：

```text
venc_channel_handler.cpp:99
```

子码流主要送 RTSP；如果录像配置不是主码流，也会送录像模块。

### 9.5 IDR 请求策略

源码：

```text
venc_channel_handler.cpp:68
venc_channel_handler.cpp:127
```

逻辑：

```cpp
if (IFrameInterval / FPS >= 5)
{
    每隔 5500ms 请求一次 IDR
}
```

目的：

- 避免录像切片时长过长。
- 避免新客户端或录像片段长时间等不到 I 帧。

RTSP 新客户端接入时也会触发 IDR：

```text
Hi3516/share/ipc_share/push_stream/rtsp/rtsp_server.cpp:108
```

## 10. VPSS AI 分支

源码：

```text
Hi3516/hi3516_ipc/main_app/stream_media/video/stream_video.cpp:853
```

`get_vpssStream()` 从 `VPSS_CHANNEL_AI` 取原始帧：

```cpp
mppVpss_get_chnFrame(VPSS_CHANNEL_AI, -1)
```

关键逻辑：

```cpp
const int process_every_n_frames = 10;
if (nFrameCount++ % process_every_n_frames == 0)
{
    algo_send_videoStreamData(...)
}
mppVpss_release_chnFrame(...)
```

解释：

- AI 分支不是每帧都处理，而是每 10 帧送一次算法，属于降载策略。
- 取到的 `ot_video_frame_info` 必须释放，否则 VPSS 通道深度会被耗尽。
- AI 通道输出 1024x576，降低算法输入尺寸。

面试表达：

```text
项目没有把主码流编码后的 H265 再解码给 AI，而是从 VPSS 单独开 AI 通道取 YUV 原始帧。
这样可以避免编解码来回损耗，同时通过降低分辨率和抽帧控制 NPU/CPU 负载。
```

## 11. RTSP 推流设计

### 11.1 推流入口

源码：

```text
Hi3516/share/ipc_share/push_stream/push_stream.cpp:130
```

`CPushStream::init()` 初始化推流模块。视频帧入口：

```text
Hi3516/share/ipc_share/push_stream/push_stream.cpp:201
```

音频帧入口：

```text
Hi3516/share/ipc_share/push_stream/push_stream.cpp:278
```

### 11.2 RTSP URL

源码：

```text
Hi3516/share/ipc_share/push_stream/rtsp/rtsp_server.h:52
```

URL 格式：

```cpp
RTSP_URL_DEFAULT "rtsp://%s:%d/Streaming/Channels/%d"
RTSP_URL_AUTHENTICATION_DEFAULT "rtsp://%s:%s@%s:%d/Streaming/Channels/%d"
```

通道：

```text
101 -> 主码流
102 -> 子码流
```

示例：

```text
rtsp://admin:password@172.16.25.36:554/Streaming/Channels/101
```

### 11.3 RTSP 初始化

源码：

```text
Hi3516/share/ipc_share/push_stream/rtsp/rtsp_server.cpp:206
```

`CRtspServer::init()` 做的事：

```text
1. 分配 LIVE_RTSP_S
2. rtsp_server_init(port, auth, user, pass, digest, dscp)
3. 获取设备 IP
4. 为 101/102 创建 Live_Stream_Info_t
5. 根据编码格式设置 RTSP_FRAMEPROTOL_H264/H265/MJPEG
6. 创建 videoQueue/audioQueue
7. 注册 rtspStateCallback 和 rtspFrameCall
8. 根据 VideoType 判断是否带音频
9. 根据音频格式设置 nAudioType
```

### 11.4 RTSP 队列

源码：

```text
Hi3516/share/ipc_share/push_stream/common/frame_queue.h:60
```

`CThreadSafeFrameQueue` 使用：

- `std::deque`
- `std::mutex`
- `std::condition_variable`
- `std::unique_ptr<FrameData>`

队列默认大小：

```text
frame_queue.h: MAX_VIDEO_FRAME = 4
frame_queue.h: MAX_AUDIO_FRAME = 4
```

但 RTSP 自己覆盖了：

```text
Hi3516/share/ipc_share/push_stream/rtsp/rtsp_server.h:42
```

通常是：

```cpp
MAX_VIDEO_FRAME = 32
MAX_AUDIO_FRAME = 16
```

### 11.5 RTSP 入队和丢帧策略

视频入口：

```text
Hi3516/share/ipc_share/push_stream/rtsp/rtsp_server.cpp:430
```

逻辑：

```text
if 客户端正在请求:
    拷贝编码帧到 FrameData
    判断是否 I 帧
    push 到 videoQueue
    如果队列满，丢弃当前帧
else:
    清空队列
```

音频入口：

```text
rtsp_server.cpp:481
```

队列满时同样丢弃当前帧。

这是一种实时流常见策略：

- 不阻塞编码线程。
- 牺牲完整性，保证实时性。
- 网络慢或客户端不取流时，不让队列无限增长。

### 11.6 新客户端 I 帧优先

状态回调：

```text
rtsp_server.cpp:108
```

新客户端开始：

```cpp
pStreamInfo->request = 1;
pStreamInfo->requestIFrame = 1;
triggerRequestIdr(nChannel);
```

取帧回调：

```text
rtsp_server.cpp:18
```

如果 `requestIFrame == 1`：

- 循环从视频队列中找 I 帧。
- 非 I 帧直接丢弃。
- 找到 I 帧后发送，并退出 I 帧等待模式。

面试解释：

```text
RTSP 客户端刚接入时，如果先收到 P 帧是无法解码的，所以项目会设置 requestIFrame 标志并主动请求 IDR。
在拿到 I 帧之前，队列里的非 I 帧会被丢弃，保证客户端首帧可解码。
```

## 12. GB28181 / SIP / RTP / PS

### 12.1 GB28181 是否使用 SIP

项目中 GB28181 使用 SIP 做信令，媒体走 RTP/PS。

关键目录：

```text
Hi3516/share/ipc_share/protocols/gb28181/sip
```

启动：

```text
Hi3516/share/ipc_share/protocols/gb28181/sip/SipModule.cpp:34
```

RTP server 初始化：

```text
Hi3516/share/ipc_share/protocols/gb28181/sip/RtpServer.cpp:111
```

### 12.2 RTP 发送线程

源码：

```text
Hi3516/share/ipc_share/protocols/gb28181/sip/RtpServer.cpp:69
```

`rtpSend_thread()`：

```text
while bRtpTheadFlag:
    如果 RTP 状态未开启，sleep 20ms
    优先取音频队列，调用 pChannel->SendMedia(..., true)
    再取视频队列，调用 pChannel->SendMedia(..., false)
    视频队列为空则 sleep 20ms
```

### 12.3 GB28181 视频入口

源码：

```text
RtpServer.cpp:185
```

主码流处理器调用：

```cpp
SIP::CRtpServer::instance()->sendVideoData(pVideoFrame);
```

`sendVideoData()` 会：

- 判断 RTP 状态。
- 分配 `Rtp_Data_info_S`。
- 拷贝编码帧。
- 队列过大时进入丢帧状态。

当前丢帧策略：

```cpp
if (list size > VIDEOFRAMEC_MAX)
{
    stRtpStream.bCurIframe = true;
}

if (bCurIframe == false)
{
    push_back(packet)
}
else
{
    free packet
}
```

这段策略比较粗糙：进入 `bCurIframe` 后会丢帧直到队列降下来，但代码片段里没有明显看到“遇到 I 帧恢复”的完整判断。面试时可以说：

```text
GB28181 RTP 侧也有队列保护，队列过大时会进入丢帧状态，避免网络阻塞拖垮编码线程。
这块如果优化，我会按 NAL 类型做更精细的 I/P 帧策略，比如保留下一帧 IDR，丢弃中间 P 帧。
```

### 12.4 PS 封装和 RTP 分包

RTP 打包入口：

```text
Hi3516/share/ipc_share/protocols/gb28181/sip/media/MediaRtp.cpp:487
```

`RTP::Packer::packRtpPackage()`：

```text
1. 先把原始 H264/H265/AAC/G711 数据封装成 PS
2. 再按 RTP MTU 分片
3. 填 RTP header
4. 设置 marker、payload type、sequence number、timestamp
```

代码中明确注释：

```cpp
/* NOTE 先封装PS格式再做RTP分包 */
```

PS 编码入口：

```text
Hi3516/share/ipc_share/protocols/gb28181/sip/media/MediaPs.cpp:1002
```

`PsEncoder::EncData()`：

- 根据音视频类型选择 stream type。
- 视频判断是否关键帧。
- 调用 `ps_muxer_input()` 生成 PS 数据。

旧的手写 PS packet 入口：

```text
MediaPs.cpp:512
```

现在 `MediaRtp.cpp` 中实际使用的是 `m_pPsEnc->EncData()`。

## 13. 音频参数和链路

### 13.1 默认音频配置

源码：

```text
Hi3516/hi3516_ipc/main_app/stream_media/audio/stream_audio.cpp:80
```

如果配置文件不存在，会写默认值：

| 参数 | 默认值 |
|---|---:|
| 音频开关 | `true` |
| 输入 | `MICIN` |
| 编码格式 | `AAC` |
| 采样率 | `16000` |
| 码率 | `48000` |
| 输入音量 | `50` |
| 降噪 | `true` |
| 输出 | `SPEAKER` |
| 输出音量 | `50` |

结构定义：

```text
Hi3516/share/ipc_share/common/define/audio_define.h:223
```

JSON 转换：

```text
Hi3516/share/ipc_share/common/convert/audio_convert.cpp:58
```

### 13.2 音频初始化流程

入口：

```text
Hi3516/hi3516_ipc/main_app/stream_media/audio/stream_audio.cpp:109
```

`CStreamAudio::init()` 流程：

```text
1. 如果 AudioSwitch=false，直接返回
2. 初始化音量原子变量
3. 注册音频配置、对讲、AO采样率、采集回调
4. stream_audio_sys_init()
5. streamAo_init() 初始化喇叭输出
6. m_streamAO.init()
7. streamAi_init() 初始化麦克风/LineIn 输入
8. streamAenc_init() 初始化 AENC
9. streamAdec_init() 初始化对讲解码
10. streamResample_init() 初始化重采样
11. bindModule() 绑定 ADEC->AO
12. 启动 deal_aiFrame_thr
13. 启动 deal_aencFrame_thr
```

### 13.3 AI 采集参数

源码：

```text
Hi3516/hi3516_ipc/main_app/stream_media/audio/ai/stream_ai.cpp:14
```

关键配置：

```cpp
stNeedParam.enBitWidth = OT_AUDIO_BIT_WIDTH_16;
stNeedParam.enSampleRate = OT_AUDIO_SAMPLE_RATE_16000;
stNeedParam.enSoundMode = OT_AUDIO_SOUND_MODE_STEREO;
stNeedParam.u32FrameNum = FRAME_NUM_DEFAULT;
stNeedParam.nVqeEnable = TD_TRUE;
stNeedParam.enVqeType = AUDIO_VQE_TYPE_TALKV2;
stNeedParam.nAoDev = AO_SPEAKER_CHN;
stNeedParam.nAoChn = AO_SPEAKER_CHN;
```

解释：

- 输入硬件采集基准是 16kHz、16bit。
- 声道模式先配置为 stereo，但在处理线程里会根据 MICIN/LINEIN 取其中一路。
- 开启 VQE TalkV2，主要用于 AEC/降噪等语音增强。
- AEC 需要知道 AO 设备和通道，因为回声消除要参考播放侧信号。

### 13.4 AENC 编码参数

源码：

```text
Hi3516/hi3516_ipc/main_app/stream_media/audio/codec/stream_aenc.cpp:13
```

默认 AENC 通道逻辑：

```cpp
if (nAencChn == AENC_AAC_CHN)
{
    enAencType = OT_PT_AAC;
    u32PointNumPerFrame = AACLC_SAMPLES_PER_FRAME;
    enSampleRate = OT_AUDIO_SAMPLE_RATE_16000;
}
```

非 AAC 通道支持：

```text
G711A -> OT_PT_G711A, 8000Hz
G711U -> OT_PT_G711U, 8000Hz
G726  -> OT_PT_G726,  8000Hz
```

但在 `CStreamAudio::init()` 中有注释：

```cpp
// note 暂不需要开启额外的编码通道
```

实际只启动 `AENC_AAC_CHN`，G.711 的实时推流是在 AI 线程里通过软件路径做。

### 13.5 AI 采集线程

源码：

```text
Hi3516/hi3516_ipc/main_app/stream_media/audio/stream_audio.cpp:857
```

`deal_aiFrame_thr()` 逻辑：

```text
1. mppAi_getFrame() 阻塞取 PCM
2. 如果输入是 LINEIN，则把右/左声道数据拷贝到目标声道
3. 清空另一路声道
4. volume_adjust()
5. mppAenc_sendFrame() 送 AENC
6. algo_send_audioStreamData() 送音频算法
7. 如果配置 G711A/G711U:
   7.1 16k -> 8k 重采样
   7.2 encode_g711()
   7.3 CPushStream::sendAudioData()
8. mppAi_releaseFrame()
9. sleep 1ms
```

设计重点：

- AI 原始帧拿到后，不是只给 AENC，还会给算法和 G.711 软件编码路径。
- G.711 因为通常要求 8kHz，项目里从 16k 采集数据重采样到 8k。
- 音频帧释放非常关键，未 release 会导致 AI 缓冲耗尽。

### 13.6 AENC 取流线程

源码：

```text
Hi3516/hi3516_ipc/main_app/stream_media/audio/stream_audio.cpp:1035
```

`deal_aencFrame_thr()` 逻辑：

```text
1. mppAenc_getFrame(timeout=500ms)
2. 检查 stream 是否为空
3. 检查 AAC ADTS 头 0xFFF
4. 根据 protection_absent 判断 ADTS 长度 7 或 9
5. cacheVoiceComFrame()
6. createFrame(stream + ADTSLen, len - ADTSLen)
7. 如果当前配置是 AAC，送 CPushStream::sendAudioData()
8. 如果正在录像，送 CStreamServer::sendAudioData()
9. freeFrame()
10. mppAenc_releaseFrame()
```

为什么要去 ADTS：

```text
RTSP/RTP 发送 AAC 时通常需要 AAC raw payload，音频参数通过 SDP 或外层协议描述。
ADTS 是文件流格式常用头，直接塞进 RTSP payload 可能导致客户端解析异常。
```

## 14. SDK/TVSDK 中的音视频配置边界

注意：实时码流不是通过 `NET_TV_GetDevConfig()` 直接返回的，SDK 配置接口主要负责“读写参数”。实时预览通常拿 RTSP URL 后走 RTSP。

### 14.1 视频编码配置 GET/SET

源码：

```text
Hi3516/share/ipc_share/protocols/tvsdk/src/callbacks/tvsdk_callbacks.cpp:635
Hi3516/share/ipc_share/protocols/tvsdk/src/callbacks/tvsdk_callbacks.cpp:666
```

`cb_get_stream_cfg()`：

```text
execute_get_result(AC_GET_VIDEO_CONFIG)
-> Convert::to_struct(...)
-> FindVideoConfigById(...)
-> TvSdkConvert::FillVideoEncodeOption(...)
```

`cb_set_stream_cfg()`：

```text
SDK NET_TV_VIDEO_ENCODE_OPTION_S
-> TvSdkConvert::ToVideoConfig(...)
-> wrap_data_json(...)
-> s_taskManage->execute(AC_SET_VIDEO_CONFIG)
```

转换函数：

```text
Hi3516/share/ipc_share/protocols/tvsdk/src/convert/tvsdk_convert.cpp:397
Hi3516/share/ipc_share/protocols/tvsdk/src/convert/tvsdk_convert.cpp:417
```

### 14.2 RTSP URL 获取

源码：

```text
Hi3516/share/ipc_share/protocols/tvsdk/src/callbacks/tvsdk_callbacks.cpp:2681
```

SDK 获取 RTSP URL 时，会根据 `NET_TV_LIVE_STREAM_INDEX_MAIN/AUX` 映射到 RTSP 主/子通道。

### 14.3 音频配置 GET/SET

源码：

```text
Hi3516/share/ipc_share/protocols/tvsdk/src/callbacks/tvsdk_callbacks.cpp:3179
Hi3516/share/ipc_share/protocols/tvsdk/src/callbacks/tvsdk_callbacks.cpp:3203
```

转换函数：

```text
Hi3516/share/ipc_share/protocols/tvsdk/src/convert/tvsdk_convert.cpp:2975
Hi3516/share/ipc_share/protocols/tvsdk/src/convert/tvsdk_convert.cpp:2989
```

面试表达：

```text
SDK 层不是直接操作 MPP，而是把 NET_TV_* 结构体转换成 IPC 内部配置结构，再通过 ActionCode 进入业务任务。
这样能隔离第三方 SDK 协议和设备内部配置模型，后续换协议或加 HTTP 网关时复用内部 ActionCode。
```

## 15. 内存和线程模型

### 15.1 硬件内存

视频裸帧主要在 MPP/VB 内部流转：

```text
VI/VPSS/VENC 使用海思 MPP 的 VB 池和模块内部缓存。
CPU 只有在取编码流、取 AI 原始帧、推流入队时接触数据。
```

关键点：

- `mppVenc_get_stream()` 后必须 `mppVenc_release_stream()`。
- `mppVpss_get_chnFrame()` 后必须 `mppVpss_release_chnFrame()`。
- `mppAi_getFrame()` 后必须 `mppAi_releaseFrame()`。
- `mppAenc_getFrame()` 后必须 `mppAenc_releaseFrame()`。

### 15.2 软件帧对象

视频帧创建：

```text
Hi3516/hi3516_ipc/main_app/stream_media/video/stream_video.cpp:736
```

音频帧创建：

```text
Hi3516/hi3516_ipc/main_app/stream_media/audio/stream_audio.cpp:727
```

实现上会分配“结构体 + 数据”的连续内存，再拷贝编码数据。

需要注意的风险：

```text
当前 createFrame() 使用 malloc 分配，但 freeFrame() 里用 delete 释放。
这是 C/C++ 内存分配释放不匹配风险，严格来说应该 malloc/free 或 new/delete 配套。
```

这个点面试时不建议主动展开太多，但如果被问“你 Review 发现过什么问题”，可以作为一个真实工程风险讲。

### 15.3 线程列表

视频线程：

| 线程 | 源码 | 职责 |
|---|---|---|
| `get_vencStream` | `stream_video.cpp:785` | 每个 VENC 通道取编码流 |
| `get_vpssStream` | `stream_video.cpp:853` | 从 VPSS AI 通道取 YUV 帧给算法 |
| RTSP 内部线程 | live555 封装库 | 客户端连接、取帧发送 |
| GB28181 RTP 线程 | `RtpServer.cpp:69` | 从 RTP 队列取媒体并发送 |

音频线程：

| 线程 | 源码 | 职责 |
|---|---|---|
| `deal_aiFrame_thr` | `stream_audio.cpp:857` | 采集 PCM、音量处理、送 AENC/算法/G711 |
| `deal_aencFrame_thr` | `stream_audio.cpp:1035` | 取 AAC 编码帧、去 ADTS、推流/录像 |

同步方式：

- 视频取流线程使用 `std::atomic` 标志控制退出。
- 音频线程也用 `std::atomic` 标志控制退出。
- RTSP 队列用 `mutex + condition_variable`。
- 音频配置更新、AO 采样率切换使用 `std::mutex` 控制。

### 15.4 性能关键点

本项目里已做的优化：

1. VPSS-VENC Wrap：降低主码流内存占用和 DDR 压力。
2. VPSS AI 分支降分辨率：AI 只吃 1024x576。
3. AI 抽帧：每 10 帧送一次算法。
4. RTSP 有界队列：队列满直接丢帧，避免推流阻塞编码线程。
5. 新客户端请求 IDR：缩短首帧可播放时间。
6. AAC 去 ADTS：提高 RTSP 音频兼容性。
7. G.711 重采样：满足 8k 电话音频常规要求。

潜在优化点：

1. `createFrame()` 拷贝编码帧到软件内存，RTSP 再拷贝一次到队列，存在多次拷贝。
2. `malloc/delete` 不匹配需要修复。
3. GB28181 丢帧策略可以更精细，建议识别 I/P 帧，丢 P 保 I。
4. RTSP 队列满时当前丢当前帧，也可以改成“丢旧 P 帧保新 I 帧”。
5. `get_vencStream()` 中整个 pack 处理加 `m_mutexSendData`，如果下游阻塞可能影响同通道处理。

## 16. 面试追问准备

### 16.1 你们为什么用 VPSS，而不是 VI 直接给 VENC？

回答：

```text
因为项目需要多路输出。主码流、子码流、AI 检测、JPEG 抓拍的分辨率和用途不同。
VI 只负责采集，VPSS 负责缩放、格式转换、低延迟、Wrap 和多通道分发。
如果 VI 直接给 VENC，就很难同时满足 2.5K 主码流、D1 子码流和 1024x576 AI 输入。
```

### 16.2 VPSS-VENC Wrap 解决什么问题？

回答：

```text
Wrap 是 VPSS 到 VENC 的卷绕缓冲模式，主要解决高分辨率编码时内存占用和 DDR 带宽压力。
项目主码流是 2880x1620，直接多帧缓存会很吃内存。
代码里 streamSys_compute_vpssVencWrap() 根据 sensor、帧率、主码流尺寸和小码流尺寸计算 wrap buffer，再放进 VB common pool。
VPSS 主通道和 VENC 主通道都开启 Wrap，减少帧搬运和延迟。
```

### 16.3 为什么 AI 从 VPSS 取帧，而不是从编码后码流取？

回答：

```text
AI 需要的是 YUV/RGB 原始图像，不适合用 H265 码流。
如果从编码后码流再解码，会多一次编解码延迟和 CPU/NPU 开销。
项目直接从 VPSS AI 通道取 1024x576 YUV 帧，并且每 10 帧处理一次，能控制算法负载。
```

### 16.4 RTSP 为什么要等 I 帧？

回答：

```text
客户端刚接入时如果先收到 P 帧，没有参考帧就无法解码。
所以 RTSP 状态回调里会设置 requestIFrame 并触发 request_idr。
rtspFrameCall() 在 requestIFrame 状态下会丢弃非 I 帧，直到找到 I 帧再发送给客户端。
```

### 16.5 音频为什么 AAC 要去 ADTS？

回答：

```text
AENC 输出的 AAC 常带 ADTS 头，适合文件流。
RTSP/RTP 通常通过 SDP 描述 AAC 参数，payload 里传 AAC raw data。
所以项目在 deal_aencFrame_thr() 中检查 0xFFF 同步字，根据是否带 CRC 判断 ADTS 头长度为 7 或 9，然后剥掉 ADTS 再推 RTSP。
```

### 16.6 SDK 设置视频参数后怎么影响 VENC？

回答：

```text
SDK 侧发 NET_TV_SET_STREAMCFG，IPC 的 tvsdk_callbacks.cpp 进入 cb_set_stream_cfg。
回调里把 SDK 的 NET_TV_VIDEO_ENCODE_OPTION_S 转成内部 VideoConfig_S，再通过 AC_SET_VIDEO_CONFIG 进入任务管理。
业务配置更新后触发 CAVConfigure 注册的视频配置回调，最终 CStreamVideo::setVideoConfig 重配对应 VPSS/VENC。
```

### 16.7 项目里如何保证实时性？

回答：

```text
第一，硬件链路通过 VI/VPSS/VENC 绑定和 Wrap 减少 CPU 参与。
第二，RTSP 使用有界队列，满了就丢帧，不阻塞编码线程。
第三，新客户端接入时主动请求 IDR，保证快速出图。
第四，AI 分支降低分辨率并抽帧处理，避免算法拖慢主码流。
第五，音频和视频各自独立线程，采集、编码、发送解耦。
```

## 17. 一句话总结

这个项目的音视频核心不是“调用几个 MPP API”，而是一套围绕实时性设计的 IPC 媒体系统：

```text
用 VI/VPSS/VENC/AENC 等硬件模块承担重负载，
用 VPSS 多通道拆分主码流、子码流、AI、抓图，
用 Wrap 和 VB 池降低内存和延迟，
用独立线程和有界队列解耦编码与网络，
用 RTSP/GB28181/SDK 把媒体能力暴露给客户端和平台。
```

