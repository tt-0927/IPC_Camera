# 统一检测层（detection/）

AI 检测的新链路。视频帧从 `main_app/ai_app/interface/algo_stream_deal.cpp` 进入 `CUnifiedDetectionEngine`，由引擎统一做节流、取帧、缩放和线程调度；各检测模型实现 `IDetectionModel` 接口，只负责自身模型的生命周期、推理和事件判断。

与 Legacy 链路（`algorithm_mode/algorithm/` 下各 detect 实现）按画像能力宏二选一编译，同一型号固件内只会运行一套。

## 数据流

```text
视频帧（stream_media 分发）
  -> algo_stream_deal.cpp            能力宏 IPC_CAP_UNIFIED_DETECTION_* 选链路
  -> CUnifiedDetectionEngine         Worker 线程：节流去重 + 帧准备 + 按序调度模型
  -> 各检测模型 process()             推理 + 事件判断
  -> tracker/（需要跟踪的模型）        跨帧 trackId + NEW/TRACKING/PREDICTED 状态过滤
  -> 事件 / 抓拍 / OSD
```

## 目录结构

| 目录 / 文件 | 职责 |
| --- | --- |
| `detection_engine.hpp/.cpp` | 三件套：`IDetectionModel` 模型接口、`CFramePreparer` 帧裁剪缩放、`CUnifiedDetectionEngine` 引擎核心 |
| `motion_detect/` | 移动侦测（svp_md）：`model` 适配器 + `logic` 事件判断 |
| `hide_detect/` | 遮挡侦测（svp_od）：`model` 适配器 + `logic` 事件判断 |
| `face_detect/` | 人脸链路：`face_detect_model` 编排 + `face_types.hpp` 域类型（FaceTarget_S 带 trackId）+ `bestshot_hub` 取优枢纽（链路级评分选优，监听器扇出）+ `capture/` 抓拍处理器 + `compare/` 比对适配层 + `worker/` 推理 Worker（capture/worker 为同名超集类独立实现）+ `library/` 名单库服务接口（预留） |
| `exhibition_detect/` | 展馆人流统计 / 人员密度：`model` + `logic` + `result_converter` 结果转换 + `density_output_executor` 输出执行 |
| `inference/` | 推理引擎抽象：`i_inference_engine.hpp` 契约 + `yolo_inference_engine` 实现 + `hiai_detect_inference_engine`（预留） |
| `tracker/` | `CDetectionTracker` 通用目标跟踪（基于 ai_share cIouTracker，改进了框语义与状态机） |
| `detection.cmake` | 构建收录 |

## 模型目录的通用结构

各检测模型目录按"适配器 + 规则"拆分：

- `*_model.hpp/.cpp`：实现 `IDetectionModel`，管理推理引擎生命周期，把检测结果交给事件层。
- `*_logic.hpp/.cpp`：抓拍规则与事件判断（区域、屏蔽区、灵敏度），不依赖引擎调度。

`face_detect` 与 `exhibition_detect` 较复杂：前者是完整人脸链路——`face_chain_types.hpp` 定义域类型（跟踪 id 随目标流转，上下文不含执行器指针），`bestshot_hub` 为链路级取优枢纽（模型装配评分后端、抓拍驱动喂入**全部规则通过目标**（每 track 独立 best 桶），"新 best"经监听器扇出；比对提特征与实时入库下期接入同一枢纽），并下辖 `capture/`（抓拍：规则过滤/事件/**多目标全部出图**（_1.._N 序号落盘，每目标优先取优 best 缓存）/SDK 逐张推送，上传与邮件取第一张）、`compare/`（比对适配层：多目标按置信度降序**依次逐个**与名单库比对，事件由 Legacy 状态机节流）、`worker/`（异步推理，含 ArcFace 按需加载）与 `library/`（名单库服务纯接口，入库仍走 Legacy 临时实例路径）——capture/worker 类名与 Legacy 同名（超集），总开关=1 时 `detection.cmake` 剔除 Legacy 侧同名源并以 `BEFORE` 头文件路径接管，保证全编译单元单一定义；后者多出结果转换与输出执行两个文件。

## 能力宏与迁移状态

`cmake/device_profiles.cmake` 按画像设置宏值，`detection.cmake` 据此决定是否编译检测层源文件：

| 宏 | 作用 | 当前状态 |
| --- | --- | --- |
| `IPC_CAP_UNIFIED_DETECTION` | 统一检测层新链路总开关：=0 全部算法走 Legacy、检测层不编译进固件；=1 MD/OD/FACE 一并切新链路 | TV-3852TL / TV-3852HL / TV-3852HZT 已开，其余画像 0 |
| `IPC_CAP_AI_EXHIBITION_PEOPLE_FLOW` | 展馆人流统计自研模型，二级开关：总开关=1 且本宏=1 才编译展馆模型 | 仅 TV-3852HZT 为 1 |
| `IPC_CAP_AI_PEOPLE_DENSITY_PIPELINE` | 人员密度输出执行器，依赖展馆宏=1 | 仅 TV-3852HZT 为 1 |

未迁移算法（人流统计、人员密度、事件管线等）继续走 `algorithm/` 下的 Legacy 链路，迁移完成后随总开关纳入。宏组合校验在 `ipc.cmake`：展馆宏=1 要求总开关=1，密度宏=1 要求展馆宏=1。

## 修改指引

- **改帧调度、节流、执行顺序**：动 `detection_engine.hpp/.cpp`，影响所有已迁移模型（MD、FACE）。改前先确认各模型的 `nOrder`、`nStartupDelayMs` 依赖。
- **新增检测模型**：新建目录实现 `IDetectionModel`（model + logic 两件套），画像加能力宏，`algo_stream_deal.cpp` 接线注册。
- **改抓拍取优**：评分选优在公共模块 `share/ipc_share/bestshot/`、链路枢纽在 `face_detect/bestshot_hub.cpp`（装配点 `face_detect_model.cpp` 的 `init()`，注释 `m_bestshotHub.init` 即关取优）；抓拍的 best 小图缓存在 `capture/` 的取优监听器。
- **改人脸抓拍处理器 / 推理 Worker**：动 `face_detect/capture/`、`face_detect/worker/`（本层独立实现）。Legacy `algorithm/face_detect/` 保持导入基线不动，新优化只落本层。
- **改人脸比对**：动 `face_detect/compare/`（适配层）。当前内部委托 Legacy `algorithm/face_detect/internal/processors/feature/`（未改动的共用件）；原生比对迁移（最优帧提特征、按 track 比对状态）时替换适配层内部实现，链路接口不变。
- **改名单库（目标库）**：接口见 `face_detect/library/face_library_service.hpp`（本期纯接口）。入库现状走 Legacy 临时实例路径（`algo_stream_deal.cpp`）；数据层 `AIFaceManage`/`CFaceSqlite` 为全链路共用，不经本层。
- **改跟踪**：动 `tracker/`。`face_detect` 已接入；`exhibition_detect` 仍直接用 ai_share cIouTracker，切换到 `CDetectionTracker` 是排队任务。
