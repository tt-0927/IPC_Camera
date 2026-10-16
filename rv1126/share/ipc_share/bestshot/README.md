# 抓拍取优模块（bestshot）

跨帧挑出每个跟踪目标质量最好的抓拍图。模块对候选帧做清晰度评分，按"清晰度 + 置信度 + 面积"综合打分，分数明显更高才替换该目标当前的最优帧（best）。模块只做"评分 + 选优"；裁剪框换算、JPEG 编码、best 小图缓存由调用方负责。

纯软件实现，不依赖任何平台头文件，ipc_share 各型号产品共用。

## 怎么用

以人脸抓拍（`hi3516_ipc` 的 `CFaceCaptureProcessor`）为例，五个调用覆盖完整生命周期：

```cpp
// 1. 装配：注入评分后端并启用取优（业务初始化时调用一次）。
//    不调用 init 则整体旁路：feed 恒返回无 best，抓拍回退当帧出图，事件链路不受影响。
Bestshot_NS::CCpuSharpnessScorer::Config_S stScorerCfg;
stScorerCfg.nGradThreshold = 30;
m_bestShotManager.init(std::make_unique<Bestshot_NS::CCpuSharpnessScorer>(stScorerCfg));

// 2. 喂候选帧：模块内部评分选优。
//    stFrame 为 Y 平面视图（虚拟/物理地址 + stride），stInput 为裁剪框 + 置信度 + 面积 + 时间戳
//    （+ 可选的 ROI 平均亮度：有平台亮度统计时填 [0,255]，无则留默认 -1，评分回退三维）。
Bestshot_NS::BestShotFeedResult_S stResult = m_bestShotManager.feed(nTrackId, stFrame, stInput);
if (stResult.bNewBest)
{
    // 本帧是该目标新 best，此时编码缓存小图（帧生命周期不跨帧）
}

// 3a. 目标结束（跟踪结束，或出图取走 best）：清该目标的桶
m_bestShotManager.onTargetEnd(nTrackId);

// 3b. 无目标帧定期检查保持窗口：返回过期的 trackId，调用方同步清理关联缓存
for (const int64_t nTrackId : m_bestShotManager.expireAll(llNowMs)) { /* 清缓存 */ }

// 4. 事件结束：全部清空，下一轮事件重新选优
m_bestShotManager.clearAll();

// 5. 释放（随析构自动执行，也可显式调用）
m_bestShotManager.deinit();
```

三种运行形态，由装配点一行决定：

| 形态 | 装配方式 | 行为 |
| --- | --- | --- |
| CPU 评分（默认） | `init(std::make_unique<CCpuSharpnessScorer>(...))` | Sobel 梯度评分，见下文 |
| 硬件评分 | 注入平台的 `ISharpnessScorer` 适配器（如 `CIveSharpnessAdapter`） | 接口相同，替换即可；IVE 因与 NPU 硬件冲突暂不默认装配 |
| 旁路 | 不调用 init | feed 恒返回无 best，调用方回退当帧出图 |

## 目录结构

| 文件 | 职责 |
| --- | --- |
| `bestshot_types.h` | 公共类型：矩形、帧视图、候选输入、feed 结果、配置 |
| `sharpness_scorer.h` | 评分后端接口 `ISharpnessScorer`，后端可插拔的契约 |
| `cpu_sharpness_scorer.h/.cpp` | CPU 软件后端：Sobel 3x3 梯度，幅值超阈值的像素占比即评分 |
| `best_shot_selector.h/.cpp` | 单目标选优器：综合评分 + 滞回更新 + 保持窗口超时失效 |
| `bestshot_manager.h/.cpp` | 对外入口 `CBestShotManager`：持有后端，按 trackId 分桶选优 |
| `bestshot.cmake` | 构建收录，由 `ipc_share.cmake` include |

## 评分与选优规则

- 亮度未知（`fBrightness = -1`，查询失败或平台未接入亮度统计）：综合分 = 清晰度 0.7 + 置信度 0.2 + 面积 0.1（面积按 300x300 像素归一化）。清晰度评分失败（返回负值）时该维度按 0 分参与，置信度主导，链路不中断。
- 亮度已知（调用方填 ROI 平均亮度 [0,255]）：
  - 硬门限：平均亮度低于 `fBrightnessMin`（默认 40，过暗）或高于 `fBrightnessMax`（默认 220，过曝）的帧一票否决，不参与评选、不刷新保持窗口（`bGated = true` 供调用方日志）。**首帧保底**：该目标还没有任何候选帧时放行（`bGateBypassed = true`）——人可能整个窗口都逆光/过曝，中途离开也要有图可出，后续光照恢复后正常帧分数远超滞回门槛，会自然替换保底帧；
  - 综合分 = 0.6×(清晰度×暗光降权) + 0.2×置信度 + 0.1×面积 + 0.1×亮度项。亮度低于 `fDimLightTh`（默认 60）时清晰度按亮度比例降权——夜间高增益噪声会让梯度占比虚高，暗处清晰度不能全信；亮度项以灰度 127 为满分，过暗过曝趋向 0 分。
- 滞回切换：新帧综合分须超过 `best x (1 + fHysteresis)`（默认 10%）才替换，防止分数抖动导致反复编码缓存。
- 保持窗口：超过 `llHoldWindowMs`（默认 5 秒）没有新候选，best 失效——目标已离开，不拿旧图出事件。
- CPU 评分语义：ROI 内 Sobel 梯度幅值超过阈值（`nGradThreshold`，默认 30）的像素占比，范围 [0,1]；运动模糊帧占比骤降。ROI 超过 `unMaxRoiSide`（默认 512）时中心截取控算力，`unSampleStep` 可隔行采样再降一半算力。

## 调用契约

- **单线程**：全部接口在同一根线程调用（检测回调线程），模块内部无锁；跨线程由调用方保证互斥。
- **资源上界**：选优桶数量上限 `unMaxTargets`（默认 32，每桶约百字节）。达到上限后新目标不参与取优（feed 返回无 best），已有桶继续选优。
- **零日志依赖**：`feed` 的返回结构携带本帧清晰度、综合分、best 分明细，调试日志由调用方按需打印。
- **trackId 为 -1**：各目标共用同一桶，兼容未接目标跟踪的调用方。

## 新增评分后端

实现 `ISharpnessScorer::eval(const BestShotFrameView_S &, const BestShotRect_S &)`：返回 [0,1] 评分，失败返回负值。帧视图同时携带 Y 平面虚拟地址（软件后端读）与物理地址（硬件后端用），各取所需。后端实现自行处理对齐与越界约束，ROI 有效性（过小等）由各后端按自身下限判断。
