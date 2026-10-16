# event - 事件处理模块

## 概述

event 是 IPC 共享的事件处理模块，当前包含 AI 检测结果标准化与事件分发 Pipeline。采用分层架构将平台无关的事件处理核心与设备厂商实现解耦，支持 Hi3516、RV1126B 等多平台复用。

## 目录结构

```
event/
├── event.cmake               # 模块 CMake 入口
├── README.md                 # 本文件
└── pipeline/                 # AI 检测结果 Pipeline
    ├── pipeline.cmake        # 子模块 CMake
    ├── core/                 # 核心类型：检测结果、处理器合同、分发器
    ├── geometry/             # 归一化几何内核（点、线段、多边形）
    ├── output/               # 平台抽象端口（IFrameImageProvider）
    └── process/              # 事件业务处理器
        ├── face/             # 人脸侦测（FACE，置信度+区域判定报警）
        ├── people_flow/      # 人流统计（PERSON，跨线计数）
        ├── people_density/   # 人员密度（HEAD，区域瞬时计数）
        ├── region/           # 区域事件（入侵/进入/离开）
        ├── trip_line/        # 越界侦测
        └── dwell/            # 驻留侦测（徘徊/停车）
```

## Pipeline 分层

```text
平台检测结果
  -> Converter（平台适配，唯一接触厂商类型）
  -> const DetectionBatch_S（平台无关标准结果）
  -> CResultDispatcher（同步、确定性分发）
  -> IResultProcessor（事件业务处理器）
  -> ProcessorOutput_S（统计/事件/OSD/图片请求）
  -> 平台输出执行器（Reporter / 联动 / OSD / 图片）
```

## 关键设计

- **三尺寸坐标合同**：标准结果同时记录源帧、模型输入、原生结果坐标三种尺寸，不对坐标语义做隐含假设。
- **事务输出**：每个处理器的输出写入临时对象，成功后才合并到帧总输出；单个处理器失败不阻断其他处理器。
- **处理器类型掩码语义**：分发器在批次中不存在掩码类型目标时**整体跳过**该处理器。依赖逐帧事件条件驱动报警状态机的处理器（如人员密度：人数清零帧必须输出 false 条件结束事件）掩码应置 0（不限制），类型过滤在处理器内部完成；仅统计不依赖空帧的处理器（如人流统计）可用精确掩码。
- **多处理器共存**：输出草稿/图片请求携带 `nStatisticsType`（对齐 `EventStatistics_NS::StatisticsType_E`），输出执行器按类型分流，报告序号空间重叠也不错配。
- **LEGACY / NEW 迁移模式**：`IPC_CAP_AI_PEOPLE_FLOW_PIPELINE`（人流统计）、`IPC_CAP_AI_PEOPLE_DENSITY_PIPELINE`（人员密度）CMake 变量控制编译模式；`0` 为旧处理器、`1` 为新框架，旧链路以 `&& !CAP_*_PIPELINE` 编译关闭。

## 适用边界：不进本模块的链路

本模块只做"检测结果进、报警意图出"的判定：处理器拿到的 `DetectionBatch_S` 只有归一化目标框、置信度和帧元数据，全程不接触检测帧像素；需要图片时输出 `ImageRequest_S` 意图，由平台输出执行器在当前帧上编码。

判定中途要频繁读取或加工检测帧像素的链路不进本模块，典型是人脸抓拍和人脸比对：

- 抓拍在判定中途逐帧统计人脸区域平均亮度，裁剪编码并缓存每个目标最清晰的一帧小图；
- 比对要在最清晰的一帧上裁剪缩放后提取特征向量。

这些操作发生在判定过程中途，不是事后出图。硬塞进 `IResultProcessor` 会把一条链拆到处理器、像素端口、端口实现、输出执行器多处，并破坏"处理器不碰像素"的约定。

这类链路的跨平台公共逻辑放独立模块（如 `share/ipc_share/bestshot/` 取优），VGS/VENC、NPU 特征提取等平台能力由业务仓实现后经接口传入。人脸抓拍/比对新链路在 `hi3516_ipc/main_app/ai_app/algorithm_mode/detection/face_detect/`，2026-10 评审结论：不迁入本模块。

## 平台适配与投产状态

启用开关在型号画像里配置（`hi3516_ipc/cmake/device_profiles.cmake`）：`IPC_CAP_UNIFIED_EVENT_PIPELINE` 是总开关，人流统计（`IPC_CAP_AI_PEOPLE_FLOW_PIPELINE`）、人员密度（`IPC_CAP_AI_PEOPLE_DENSITY_PIPELINE`）另有各自迁移开关。目前只有展馆型号 TV-3852HZT 打开了全部开关并投入使用；其余 Hi3516 型号开关全为 0，事件仍走 Legacy 旧链路。

| 平台/型号 | Converter | 组合根 | 已迁移处理器 | 使用状态 |
|------|------|------|------|------|
| Hi3516 · TV-3852HZT（展馆 YOLO） | CExhibitionResultConverter | CExhibitionDetectModel | 人流统计、人员密度 | 已投入使用（唯一启用的型号） |
| Hi3516 · HVF 常规型号 | CHisiHvfResultConverter | CHVFEventMigrationController、CHVFPeopleFlowMigrationController | 区域（入侵/进入/离开）、越界、驻留（徘徊/停车）、人脸侦测报警、人流统计 | 代码已迁移，未投入使用（画像开关为 0，走 Legacy） |
| RV1126B | 未开发 | 未开发 | 无 | 未接入 |

两条 Hi3516 链路共用图片端口实现 CHisiFrameImageProvider。人脸抓拍、人脸比对不进本模块（见"适用边界"）。

## 模块开发流程

### 新增事件处理器

1. 先对照"适用边界"判断：判定中途不碰检测帧像素的链路才进本模块。
2. 在 `pipeline/process/<事件名>/` 建目录，按现有处理器起三件套：`<事件>_types.hpp`（平台无关配置与类型，配置坐标按 1920×1080 像素基准）、`<事件>_config_adapter.{hpp,cpp}`（归一化到 [0,1]）、`<事件>_processor.{hpp,cpp}`（实现 IResultProcessor，输出只写 `ProcessorOutput_S` 四类意图）。无状态参照 `face/`，跨帧状态（跟踪桶、驻留计时）参照 `dwell/`、`region/` 的 `state/` 子目录。
3. 所有文件用 `#if CAP_UNIFIED_EVENT_PIPELINE` 包裹。
4. 在 `pipeline/pipeline.cmake` 两处登记：`AI_PIPELINE_INCLUDE_PATH` 加新目录，`AI_PIPELINE_SOURCE_LIST` 逐个加新 `.cpp`（显式清单；CAP=0 型号全编译为空，漏登只在 CAP=1 型号链接期报错，务必登记完整）。
5. 平台侧配套（业务仓）：Converter 把厂商检测结果转 `DetectionBatch_S`，OutputExecutor 消费 `ProcessorOutput_S`，组合根装配；型号启用走 `device_profiles.cmake` 画像变量，新增 CAP 宏要在 `validate_and_export_caps` 里加校验。

### 代码约束

- C++11 基线（`ipc_share.cmake` 已固定），不引入 `<optional>`、`string_view` 等 C++17 头。
- `dispatch`/`process` 在平台检测回调线程同步调用，处理器内不做阻塞 I/O；`apply_config` 的调用线程与 `process` 的并发关系在新处理器接口注释里写明。
- `process` 返回非 OK 时该处理器输出整体丢弃，不得留下半更新状态（先校验、后改状态）。

### 合入前要更新的信息

- 本 README 的"目录结构"树：新增或删除处理器目录时同步。
- 本 README 的"平台适配与投产状态"表：迁移了哪些处理器、哪个型号已启用。
- 本 README 的"关键设计"/"适用边界"：新增横切约定，或出现新的"不进本模块"案例时补充。
- 静态检查：`./tools/cppcheck.sh share/ipc_share/event/`，改动文件过 `clang-format -i --style=file`。