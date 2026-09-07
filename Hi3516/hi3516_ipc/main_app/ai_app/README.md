
#ai_app

AI算法检测模块。

##代码结构

        - **`algorithm_mode /`**算法模块 - **`common /`**公共函数模块 - **`interface /`**ai_app接口 -
        **`stream_proc /`**流数据处理模块

            ##AI 检测结果标准化与事件分发框架

                从 2026 -
        08 起，`share/ipc_share/event/pipeline` 提供跨平台 AI 检测结果标准化与事件分发核心，Hi3516 侧适配层位于 `algorithm_mode / algorithm / hvf_detect / internal /
            pipeline /`。

                      ## #分层架构

```text Hi3516 VPSS Frame->海思 HVF 一次推理->CHisiHvfResultConverter（平台结果转换，唯一接触 ot_aidetect_result_array
                          的组件）->const DetectionBatch_S（平台无关、归一化标准结果）->CResultDispatcher（同步、确定顺序、事务输出）
                              ->CPeopleFlowProcessor（共享人流统计业务处理器）->ProcessorOutput_S（统计草稿 /
            事件条件 / OSD / 图片请求）->CHVFPeopleFlowOutputExecutor（Hi 输出适配）->现有 Reporter / CAlarmStateMachine / CEventLinkage /
            OSD / VGS -
        JPEG
```

        ## #三尺寸坐标合同

        标准检测结果同时记录三种尺寸，禁止互相隐式替代：

        - 实际源帧尺寸（`stSourceFrameSize`） - 模型输入尺寸（`stModelInputSize`） -
        原生结果坐标尺寸（`stNativeResultSize`，几何归一化的基准）

                HVF 送帧前会把检测帧缩放到模型输入分辨率（`ss_mpi_aidetect_get_model_info` 返回的 `model_info
                    .size`），因此 `detect_rect` 确定属于模型坐标，原生结果坐标固定采用 `MODEL_INPUT`。事件处理器图片编码仍使用原始源帧，保证全景图 /
            目标图分辨率不下降。

            ## #人流统计迁移模式

            设备画像 `device_profiles.cmake` 中能力变量 `IPC_CAP_AI_PEOPLE_FLOW_PIPELINE` 决定编译期模式：

    | 取值 | 模式 | | -- - | -- - | | 0 | LEGACY（只运行旧处理器，保留原行为） | | 1 |
    NEW（只运行新框架，检测标准化 + 共享人流统计处理器） |

`TV - 3852HZT` 启用 NEW（1），其他 Hi3516 画像保持 LEGACY（0）。切换模式需修改画像并重新构建固件。

            ## #后续事件接入方式

            新事件实现 `AiPipeline_NS::IResultProcessor` 并在迁移控制器（Composition
                Root）注册到 `CResultDispatcher`，即可复用标准结果、几何内核与类型化输出；平台侧只需提供结果 Converter
                    与图片能力实现。RV1126B 适配（Converter /
            Frame Image Provider / Composition Root）待 Hi3516 整机测试通过后另建任务。

## 统一检测层（Unified Detection Layer）

从 2026-08 起，`algorithm_mode/detection/` 提供统一检测层：每通道单检测 Worker 线程，
串行执行已注册的检测模型，统一负责帧队列（latest-wins）与帧缩放/裁剪（`CFramePreparer`），
消除"每个算法一个线程 + 一个队列 + 各自缩放"的重复结构。

### 架构

```text
algo_stream_deal（帧源）
	  -> CUnifiedDetectionEngine（每通道一个，NEW 模式 MD/OD 的唯一帧接收者）
	       latest-wins 队列（容量 2）
	       单 Worker 线程（UnifiedDetect）：
	         取帧 -> CFramePreparer 准备（按模型声明的分辨率缩放，同帧同参数复用）
	              -> 按 nOrder 串行执行已注册模型（各模型自带帧率控制与跳帧）
	              -> MD/OD 走自身结果合同（现有事件处理器/报警状态机）
```

### 目录与职责

| 文件 | 职责 |
| --- | --- |
| `detection/detection_engine.hpp/cpp` | `CUnifiedDetectionEngine`：Worker、队列、模型注册/懒初始化/重初始化、失败隔离；`IDetectionModel` 接口与 `DetectionModelDescriptor_S` 模型描述；`CFramePreparer`：缩放/裁剪缓冲池与帧周期缓存 |
| `detection/motion_detect/motion_detect_logic.hpp/cpp` | `CMotionDetectLogic`：移动侦测事件判断逻辑（NEW 适配器共用） |
| `detection/motion_detect/motion_detect_model.hpp/cpp` | `CMotionDetectModel`：svp_md 适配器 |
| `detection/hide_detect/hide_detect_logic.hpp/cpp` | `CHideDetectLogic`：遮挡侦测事件判断逻辑（NEW 适配器共用） |
| `detection/hide_detect/hide_detect_model.hpp/cpp` | `CHideDetectModel`：svp_od 适配器 |

### 模型接入方式

1. 实现 `IDetectionModel`：声明描述（名称/输入分辨率/裁剪/执行顺序）、帧率控制
   `shouldProcess`、生命周期 `init/unInit` 与处理入口 `process`。
2. 引擎 `register_model()` 注册（必须在 `start()` 之前），`start()` 后由引擎 Worker
   懒初始化并串行分发；单个模型失败（初始化/帧准备/处理）不影响其他模型与引擎线程。
3. 目标框类模型后续接入时，在 `process` 内经 Converter 产出 `DetectionBatch_S` 交给
   `CResultDispatcher`；MD/OD 统计型结果走各自现有合同。

### 迁移宏

设备画像 `device_profiles.cmake` 中能力变量决定编译期模式（由 `ipc.cmake` 校验后派生
`CAP_UNIFIED_DETECTION_MD` / `CAP_UNIFIED_DETECTION_OD` 编译宏）：

| 宏 | 0（LEGACY，默认） | 1（NEW） |
| --- | --- | --- |
| `IPC_CAP_UNIFIED_DETECTION_MD` | `CMotionDetect` 独立线程/队列/缩放，行为不变 | `CMotionDetectModel` 注册进引擎 |
| `IPC_CAP_UNIFIED_DETECTION_OD` | `CHideDetect` 独立线程/队列/缩放，行为不变 | `CHideDetectModel` 注册进引擎 |

首轮全部画像保持 0（LEGACY）；基础设施经设备测试验证后按型号逐个启用。
任一宏置 0 重构建即回 LEGACY，引擎代码保留不撤销。

## 展馆人流统计/人员密度检测（Exhibition People Flow / Density）

从 2026-08 起，`algorithm_mode/detection/exhibition_detect/` 提供自研展馆
YOLOV8n 模型（`ai_exhibition.json`，1024×576，person/head 两类）接入统一检测层，
替换海思 HVF 作为人流统计检测源；统计业务复用共享 `CPeopleFlowProcessor`。
人员密度事件同样由该模型承载：head 类经独立 IOU 跟踪后交给共享
`CPeopleDensityProcessor`，替换 HVF V2 密度链路。

### 架构

```text
CUnifiedDetectionEngine（复用，每通道单 Worker）
  -> CExhibitionDetectModel（IDetectionModel 适配器）
       -> Inference_NS::CYoloUltralytics（Hisilicon，ai_exhibition.json）
       -> cIouTracker ×2（person 框 -> 人流统计 Track ID；head 框 -> 密度 Track ID；
                          轨迹容量上限各 20，满载停止建轨、高置信优先）
       -> CExhibitionResultConverter（TrackResult_S -> DetectionBatch_S，PERSON/ENDED + HEAD）
       -> CResultDispatcher + CPeopleFlowProcessor（人流统计）
                          + CPeopleDensityProcessor（人员密度，掩码 0，空帧也驱动状态机）
       -> CHVFPeopleFlowOutputExecutor（人流统计输出适配，跳过密度草稿/图片请求）
       -> CExhibitionDensityOutputExecutor（密度三级状态机 + 周期上报 + 全景图）
       -> CHisiFrameImageProvider（两个执行器共用）
       -> 现有 Reporter / CAlarmStateMachine / OSD
```

### 目录与职责

| 文件 | 职责 |
| --- | --- |
| `detection/exhibition_detect/exhibition_detect_model.hpp/cpp` | `CExhibitionDetectModel`：YOLO 推理 + 双跟踪 + 转换 + 分发 + 输出执行（模型 process 内完成 Dispatcher/Processor/Executor 组装） |
| `detection/exhibition_detect/exhibition_result_converter.hpp/cpp` | `CExhibitionResultConverter`：TrackResult_S -> DetectionBatch_S（person→PERSON+TRACKED+TrackID，消失轨迹→PERSON+ENDED；head→HEAD+TRACKED+TrackID，密度为瞬时计数不输出 ENDED；三尺寸 MODEL_INPUT 语义） |
| `detection/exhibition_detect/exhibition_detect_logic.hpp/cpp` | `CExhibitionDetectLogic`：配置路由（人流与 HVF 共用 `nEnPeopleFlowStatistics`/`Alarm::PeopleFlowStatistics_S`，密度与 HVF V2 共用 `nEnPeopleDensityDetection`/`Alarm::PeopleDensityDetection_S`）、跟踪器参数（person：max_age=5/min_hits=3/iou=0.3；head：max_age=5/min_hits=1/iou=0.3；容量上限 max_tracks=20 共用）、生效状态翻转重建请求 |
| `detection/exhibition_detect/exhibition_density_output_executor.hpp/cpp` | `CExhibitionDensityOutputExecutor`：密度草稿→`Report_S` 上报，三级密度条件→三台 `CAlarmStateMachine`，全景图按 heavy payload 构建 |

### 坐标语义

- YOLO 输出框为左上角 (nX1,nY1) + 右下角 (nX2,nY2)（`DetectionBox_S`）；
- `IouTracker` 的 `DetectResult_S` 为"中心点 (nX1,nY1) + 宽高 (nW,nH)"语义（见 IouTracker.cpp `iou()`），
  模型适配器在送跟踪器前换算，转换器还原左上角时保持同一语义。

### 能力宏

设备画像 `device_profiles.cmake` 中能力变量决定人流统计/人员密度的检测链路：

| 宏 | 0（默认） | 1 |
| --- | --- | --- |
| `IPC_CAP_AI_EXHIBITION_PEOPLE_FLOW` | 人流统计走海思 HVF，行为不变 | 展馆模型注册进统一检测引擎；HVF 人流统计编译关闭（`hvf_detect.cpp` 与 `algo_stream_deal.cpp` 中 `CAP_AI_PEOPLE_STATISTICS` 相关路径以 `&& !CAP_AI_EXHIBITION_PEOPLE_FLOW` 互斥） |
| `IPC_CAP_AI_PEOPLE_DENSITY_PIPELINE` | 人员密度走 HVF V2，行为不变 | 展馆模型 head 类接入共享密度处理器；HVF 密度链路编译关闭（`CAP_AI_PEOPLE_DENSITY_V2` 相关路径以 `&& !CAP_AI_PEOPLE_DENSITY_PIPELINE` 互斥） |

依赖关系：展馆人流 `=1` 要求 `CAP_AI_PEOPLE_STATISTICS=1`；密度迁移 `=1` 要求
展馆人流 `=1`（ipc.cmake 校验 + 头文件 `#error` 守卫）。首轮仅展厅画像
（TV-3852HZT）启用；宏置 0 重构建即回旧链路，新代码保留不撤销。
