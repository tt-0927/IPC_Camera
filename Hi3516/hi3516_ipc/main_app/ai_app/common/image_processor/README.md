# image_processor：算法侧图片编码公共组件

给算法事件提供"帧 → JPEG"的统一编码能力，替代各算法自己写裁剪和编码。
目录下两个文件，分工是底层服务和上层封装：

| 文件 | 职责 | 什么时候用 |
| --- | --- | --- |
| `jpeg_encode_service.hpp/.cpp` | VENC 硬件 JPEG 编码服务（单例 `CJpegEncodeService`）：全景整帧编码、批量抠图编码、通道生命周期 | 手里有 `ot_video_frame_info` 帧和目标框，要 JPEG 字节 |
| `video_frame_jpeg_encoder.hpp/.cpp` | 上层封装（自由函数）：帧编码后写文件、或送 TVSDK 事件图（有客户端订阅才编码） | 算法事件出图、落盘、上报 |

`video_frame_jpeg_encoder` 内部优先走 `CJpegEncodeService` 硬件路径，
不满足硬件约束时回退 FFmpeg 软编码（`ffmpeg_image.h`），调用方不用挑选路径。

## 快速使用

### 场景一：一批目标框 → 一批小图（事件裁剪小图的标准用法）

```cpp
#include "jpeg_encode_service.hpp"

/* 输入：AI 通道源帧 + 已逆变换到源帧坐标、已对齐的目标框数组 */
std::vector<ot_rect> vecRects;                  /* 源帧 1920x1080 坐标 */
std::vector<std::vector<uint8_t>> vecJpegs;     /* 输出：与 vecRects 下标一一对应 */

if (AiAppCommon::CJpegEncodeService::instance()->encodeCrops(*pSrcFrameInfo,
                                                             vecRects, vecJpegs) == OK)
{
    /* vecJpegs[i] 即第 i 个目标的 JPEG 字节，写文件/进事件附件 */
}
```

### 场景二：整帧全景图

```cpp
std::vector<uint8_t> vecJpeg;
AiAppCommon::CJpegEncodeService::instance()->encodePanorama(*pFrameInfo, vecJpeg);
```

### 场景三：编码并落盘 / 送 TVSDK 事件图

```cpp
#include "video_frame_jpeg_encoder.hpp"

/* 编码一帧写 JPEG 文件（内部自动选硬/软路径） */
AiAppCommon::encode_video_frame_to_jpeg_file(&stFrameInfo, strFilename);

/* 送 TVSDK 事件图：无客户端订阅时不编码直接跳过（先调 tvsdk_event_image_required 判断） */
if (AiAppCommon::tvsdk_event_image_required())
{
    EventTvSdkImage_S stImage;
    AiAppCommon::encode_video_frame_to_jpeg_memory(&stFrameInfo, stImage);
}
```

## 硬约束（不满足时接口返回失败，不会崩溃）

硬件路径（`encodePanorama` / `encodeCrops`）来自 VENC `batch_crop` 的硬性要求：

- 源帧必须 **1920x1080、非压缩 NV21**（即 VPSS AI 通道原始帧；统一检测引擎的
  `stCtx.pSrcFrameInfo` 天然满足）；
- 抠图框 **x 8 像素对齐、y 2 像素对齐、宽高不小于 32x32、不越出源帧**；
  调用方负责对齐（参考 `face_capture_processor.cpp` 的 `ALIGN_BACK` + clamp 写法）；
- 单次调用最多 **60 个框**（`MPP_VENC_JPEG_MAX_CROP_NUM`），更多需分批；
- 品质因数 qfactor 取 [1,99]，传 0 沿用默认 90。

检测空间（640x640）到源帧（1920x1080）的坐标逆变换属于各算法自己的映射规则，
本组件不做（各算法的 `ConvertResolution` 规则不同）。

## 行为特性

- **线程安全**：单例内部互斥锁串行编码，任意线程可调；多算法同时出图自动排队
  （单次 1080p 编码实测约 10ms，事件频率下排队可忽略；超时 500ms 失败返回）。
- **内存有界**：输出缓冲复用、只增不减，上界为历史最大单批抠图容量；
  编码通道（VENC 通道 3，码流池约 4MB MMZ）**首次编码时创建**，算法全关时由
  `algo_stream_deal.cpp` 调 `releaseChannel()` 释放，下次编码自动重建（毫秒级）。
- **错误语义**：C 风格 int 返回（0 成功），失败打 ERROR 日志，无重试。

## 新算法接入（如 hvf / 区域入侵等事件出图）

1. 在算法检测回调里：检测框逆变换到源帧坐标 → 按"硬约束"一节对齐 → 调
   `encodeCrops`（场景一示例原样可用）；
2. 落盘与数据库登记走业务侧已有的抓图管理（`capture_manage`），本组件只产 JPEG 字节；
3. **通道释放条件**：当前挂在"人脸抓拍+比对全关"（见 `algo_stream_deal.cpp` 的
   `releaseChannel` 调用处）。新算法接入后需把该条件扩展为"所有出图算法全关"，
   否则只关人脸时通道会提前释放、新算法下次出图要走重建（功能不受影响，多一次
   毫秒级重建）。

## 调试

板端 `touch /opt/cam/testJpegEncode` 后（`video_frame_jpeg_encoder` 内置开关）：

1. 无 TVSDK 客户端订阅时也临时编码（验证编码链路不依赖客户端连接）；
2. 每次编码成功把结果覆盖写到 `/tmp/jpeg_debug_latest.jpg`，并打印实际使用的
   编码后端与尺寸。

## 板端验证口径

- 多算法并开出图压测：`批量抠图硬编码失败` 日志零出现；
- `cat /proc/umap/media-mem`：算法运行期间仅一条约 4MB 的 `jpege3_stm`，
  算法全关后消失、占用回到基线；
- 编码延迟：串行互斥下单次调用约 10ms 量级（板端实测批量抠图编码耗时）。
