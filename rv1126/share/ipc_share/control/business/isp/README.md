# 图像信号处理（ISP）共享业务模块

本目录是 `ipc_share` 的 ISP 核心。它保存配置、检查参数、应用 ISP 参数，并按优先级选择场景、IR-CUT 和灯光设置；不直接调用芯片 SDK，也不读取产品宏。

平台只需提供支持的功能、四个适配接口、硬件切换时间和时钟回调，再把 `CIspBusinessService` 注册到 `CIspManage`。共享代码通过接口调用平台，因此可复用到其他芯片。

## 目录结构

```text
share/ipc_share/control/business/isp/
├── README.md
├── isp.cmake                              # 共享模块构建入口
├── isp_manage.h/.cpp                      # 对外调用入口；安全注册和清除服务
├── isp_service_interface.h                # 共享库与业务库共用的服务接口
├── capability/
│   ├── isp_capability_profile.h            # 平台支持的功能和参数范围
│   └── isp_capability_validator.*          # 能力与参数的一致性校验
├── config/
│   ├── model/isp_config_value.h            # 配置值 variant：IspConfigValue_T
│   ├── application/
│   │   ├── isp_config_repository.*         # CIspConfigure 的仓储包装
│   │   └── isp_config_command_service.*    # 校验、持久化、应用与恢复事务
│   └── storage/isp_configure.*             # ISP 配置文件读写
├── interface/
│   └── isp_platform_adapters.h             # 参数、场景、检测、外设四类适配端口
├── peripheral/
│   └── isp_peripheral_controller.*         # 将 ISP 灯光目标转换为共享物理驱动操作
├── orchestration/
│   ├── isp_business_service.*              # 创建并连接共享业务组件的入口
│   ├── param/isp_param_orchestrator.*      # 读取、校验和再次应用参数
│   ├── scene/isp_scene_orchestrator.*      # 创建、使用和释放平台场景资源
│   ├── scene/isp_scene_scheduler.*         # 每秒检查计划并提交场景请求
│   ├── daynight/isp_daynight_mode_controller.* # DAY/NIGHT/TIME/AUTO 模式控制
│   └── fill_light/isp_fill_light_orchestrator.h # 灯光切换时序模型
├── runtime/
│   ├── isp_runtime_arbiter.*               # 按优先级选择场景、日夜和临时灯光设置
│   ├── isp_runtime_reconciler.*            # 合并更新并按顺序操作硬件
│   └── model/
│       ├── isp_runtime_intent.h            # 各来源提交的设置请求
│       ├── isp_runtime_target.h            # 唯一待应用硬件设置
│       ├── isp_runtime_scene.h             # 内部日夜场景
│       ├── isp_runtime_decision.h          # 日夜选择结果
│       ├── isp_light_target.h              # 灯光目标
│       └── isp_transition_progress.h       # 硬件切换结果与错误码
└── policy/
    ├── isp_param_policy.*                  # 参数校验和范围修正
    ├── isp_daynight_policy.*               # 日夜模式选择
    ├── isp_scene_schedule_policy.*         # 计划校验与边界检测
    └── isp_replay_order.*                  # 参数重放顺序
```

## 对外入口与配置事务

业务代码通过 `CIspManage` 查询 ISP 能力、写入配置、设置场景计划、临时控制灯光和恢复默认配置。`CIspManage` 不拥有具体服务，只保存已注册 `IIspBusinessService` 的非拥有指针；`clear_business_service()` 会等待正在执行的调用结束，返回后才能销毁服务和适配器。

`CIspConfigCommandService` 是修改配置的唯一入口：先按平台支持范围校验和修正，保存旧值副本，再写入存储并应用新值。若应用失败，会按配置类别恢复旧的保存值和硬件设置。恢复“显示设置默认值”只恢复 ISP 显示相关配置，不会改写外设补光的独立配置。

## 两类场景各自负责什么

- `SceneType_E` 表示网页可选的六个**配置场景**：普通、顺光、背光、低光照、自定义 1、自定义 2。它决定参数槽位、持久化配置和场景计划。
- `IspRuntimeScene_E` 表示内部的**日夜场景**：白天、夜间白光、夜间红外、夜间关灯、夜间智能补光。它决定平台场景资源、伽马（Gamma）、IR-CUT 和补光联动。
- 网页场景变化时重放对应参数；内部场景变化时切换平台场景，再重放当前网页场景的参数。两者分别处理，互不覆盖。
- 平台场景适配器（adapter）只接收 `IspRuntimeScene_E`；禁止把网页 `SceneType_E` 当作日夜硬件模式下发。

## 怎样选择并应用硬件设置

`CIspRuntimeArbiter` 按优先级从用户场景、计划场景、日夜模式、临时灯光和补光总控中选出最终设置。`CIspRuntimeReconciler` 在单独线程中按顺序应用设置；短时间内多次更新时，只处理最后一次。业务线程不直接操作硬件。

- 硬件执行器（reconciler）只在适配器返回 `OK` 后记录 IR-CUT 和全部灯光参数的最后成功状态；下一次设置始终与该状态比较。
- 关灯、适配器失败和被新更新抢占都不会写入成功状态。外设失败会按平台提供的时间间隔有限重试，等待期间可被新更新中断。
- 切换结果（transition progress）中的错误码才是本次执行结果；“最后处理序号”（last completed generation）只表示该次已处理结束，不代表成功。
- 临时灯光也必须经过同一套选择和执行过程，不能绕过补光总控。

## 外设补光边界

外设补光配置由同级 `peripheral/` 模块独立保存和判断。ISP 仅输出画面模式、IR-CUT 与灯光请求；外设补光总控在临时灯光之后检查是否允许开灯，并限制最大功率。总控不能通过改写 ISP 日夜场景或 IR-CUT 来“关灯”，以保证白光场景保持彩色，其余夜间场景保持预期的黑白或彩色效果。

共享 `peripheral/isp_peripheral_controller.*` 负责把选出的灯光设置转换为同仓 `hardware/peripheral/` 的通用补光和 IR-CUT 驱动操作。业务仓只构造板级参数并创建该控制器，不复制 PWM、GPIO 或告警闪烁逻辑。

## 场景计划语义

- 计划调度器（scheduler）只有一个线程，每秒读取一次本地时间副本（月和当天秒数）以及当前计划副本，并重新计算命中的配置场景；手动校时、跨日和跨月不依赖额外通知。
- 网页更新计划时仅在互斥锁（mutex）保护下替换计划副本，更新最迟下一秒生效。
- 命中状态没有变化时不会提交新的设置请求；计划禁用或未命中时清除计划请求，自动回落到 `pic_scene_param.json` 的 `CurrentScene`。
- 计划调度器不启动、停止或操作自动日夜检测；日夜选择、参数再次应用和硬件重试仍由共享业务服务和硬件执行器管理。

## 平台接入与构建约束

1. 在业务仓构建平台功能和参数范围；平台能力、机型和板级宏只能在业务仓的构建器（builder）中读取。
2. 实现并装配四个端口：`IIspParameterApplier`、`IIspSceneProvider`、`IIspDayNightDetector`、`IIspPeripheralController`。
3. 由启动装配器（bootstrap）创建 `CIspBusinessService`，先注册到 `CIspManage`，再初始化；退出时先停止并清除服务，再销毁适配器与平台驱动。
4. `isp.cmake` 已把本目录的配置、策略和硬件执行源文件纳入共享构建；同级 `peripheral/peripheral.cmake` 需先于本模块引入。

共享层禁止依赖 `hi3516_ipc`、`rv1126b_ipc`、厂商 SDK（如 `ot_`、`ss_mpi_`、`rk_aiq_`）和 `CAP_*`/产品宏。新芯片平台接入步骤见 `docs/isp模块重构/ISP新芯片平台接入指南.md`。
