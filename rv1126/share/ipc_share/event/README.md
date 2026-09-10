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
        ├── people_flow/      # 人流统计（PERSON，跨线计数）
        └── people_density/   # 人员密度（HEAD，区域瞬时计数）
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

## 平台适配

| 平台 | Converter | Image Provider | Composition Root |
|------|-----------|----------------|------------------|
| Hi3516（HVF） | CHisiHvfResultConverter | CHisiFrameImageProvider | CHVFPeopleFlowMigrationController |
| Hi3516（展馆 YOLO） | CExhibitionResultConverter | CHisiFrameImageProvider | CExhibitionDetectModel（人流统计 + 人员密度） |
| RV1126B | 待开发 | 待开发 | 待开发 |