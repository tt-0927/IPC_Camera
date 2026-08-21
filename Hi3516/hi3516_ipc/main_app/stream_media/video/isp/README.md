# Hi3516 图像信号处理（ISP）平台适配目录

本目录实现 Hi3516 平台需要的 ISP 代码。程序固定使用 `ipc_share` 的 `CIspBusinessService`；这里负责读取 Hi3516 硬件配置、实现平台接口，并在启动时注册服务。没有旧实现，也没有 `USE_SHARED_ISP_CORE` 编译开关。

## 目录结构

```text
hi3516_ipc/main_app/stream_media/video/isp/
├── README.md
├── bootstrap/
│   └── isp_runtime_bootstrap.*          # 创建硬件参数、驱动、适配器和共享服务的入口
├── capability/
│   ├── isp_capability_builder.*         # 将 Hi3516 ISP 功能和参数范围写入共享数据
│   └── isp_tuning_builder.*             # 绑定编译期 Sensor 类型和静态 Gamma 资源
├── config/
│   └── isp_profile_config_loader.*      # 加载并校验参数映射与运行策略 INI
├── platform/
│   ├── isp_parameter_applier_hi3516.*   # IIspParameterApplier：参数与 Gamma 后处理
│   ├── isp_scene_provider_hi3516.*      # IIspSceneProvider：场景资源和 MPP Scene 接口
│   └── isp_daynight_detector_hi3516.*   # IIspDayNightDetector：AE/WB 观测
└── device/
    ├── isp_control.*                    # Hi3516 网页参数到 MPP 参数的映射
    ├── isp_scene.*                      # 平台场景 INI 和 MPP Scene 调用
    ├── isp_dayNight.*                   # AE/WB 统计与底层日夜检测
    └── isp_tuning_profile.h             # Hi3516 机型调参数据模型

hi3516_ipc/main_app/peripheral/
└── peripheral_profile_builder.*          # 构建补光 PWM、IR-CUT GPIO 板级参数
```

本目录不再维护 `param/`、`scene/`、`daynight/`、`fill_light/`。参数应用、场景计划、日夜切换和失败重试都在 `share/ipc_share/control/business/isp/`；补光和 IR-CUT 驱动在 `share/ipc_share/hardware/peripheral/`。

## 启动与退出流程

`stream_video.cpp` 在 `CIspControl::init()` 成功后调用 `CHi3516IspRuntimeBootstrap::init()`；在释放 `CIspControl` 之前调用其 `deinit()`。这个顺序保证共享服务运行期间，MPP 资源和调参数据仍然有效。

启动器按以下顺序工作：

1. 构建 ISP 支持的功能和参数范围，从 `ISP_CONFIG_PATH` 加载 ISP 参数映射与运行策略，并构建补光和 IR-CUT 的板级参数。
2. 将调参数据设置给 `CIspControl` 和 `CDayNightController`，再初始化共享 `CFillLightDriver` 并创建 `CIrCutDriver`。
3. 创建三个 Hi3516 平台适配器（adapter），并使用共享 `CIspPeripheralController` 把补光、IR-CUT 驱动接成第四个外设端口。
4. 设置平台时间要求（关灯稳定、IR-CUT 最小间隔、外设重试）和时钟回调，创建共享 `CIspBusinessService`。
5. 向 `CIspManage` 注册服务后执行 `CIspManage::init()`，由共享服务初始化场景资源、单线程硬件执行器（reconciler）、日夜检测和场景计划。

退出时先停止共享服务，再清除 `CIspManage` 中的服务指针，最后按相反顺序释放服务、适配器和驱动。清除失败时保留对象，避免留下无效指针。

## 四个适配边界

共享核心只通过下列端口调用平台实现：

| 端口 | Hi3516 实现 | 职责 |
| --- | --- | --- |
| `IIspParameterApplier` | `CIspParameterApplierHi3516` | 下发图像、曝光、背光、自动白平衡（AWB）、降噪、镜像参数，并在场景生效后处理伽马（Gamma）。 |
| `IIspSceneProvider` | `CIspSceneProviderHi3516` | 初始化和释放平台场景资源，按内部日夜场景切换 MPP Scene。 |
| `IIspDayNightDetector` | `CIspDayNightDetectorHi3516` | 提供自动曝光/白平衡（AE/WB）日夜观测、灵敏度设置和采样启停；不直接操作硬件。 |
| `IIspPeripheralController` | 共享 `CIspPeripheralController` | 把共享灯光目标下发到 `CFillLightDriver` 和 `CIrCutDriver`。 |

`SceneType_E` 是网页配置场景，`IspRuntimeScene_E` 是日夜硬件场景。Hi3516 的场景提供器（provider）和参数适配器只能根据后者调用 MPP Scene 接口、伽马（Gamma）或 IR-CUT 联动，不能把网页配置场景直接作为平台硬件模式。

## 维护约束

- Hi3516 硬件能力宏只允许在 `capability/isp_capability_builder.*` 消费；网页参数映射、日夜阈值、Scene/DRC、Gamma 和安装方向必须保存在 Sensor 目录的 `config_isp_param_mapping.ini`、`config_isp_runtime_policy.ini`，不得恢复机型或焦距调参宏。
- `config_isp_param_mapping.ini` 的所有网页数值映射均按白天、夜晚白光、夜晚红外三个运行场景完整配置；它承载亮度、锐度、WDR、对比度、饱和度、曝光补偿、HLC、AWB 和降噪规则。`config_isp_runtime_policy.ini` 只承载日夜阈值、Scene/DRC、Gamma 与安装方向，不能把网页数值公式放入运行策略文件。
- 网页读取以 `share/ipc_share/control/business/isp/config/storage/isp_configure.h` 的持久化配置为准；平台 `CIspControl::get_*` 不作为网页配置回显的依赖。新增网页参数时优先补齐其 `set_*` 的 INI 映射与启动期校验。
- `isp_tuning_builder.*` 只允许保留编译期 Sensor 驱动类型和静态资源绑定；运行时配置不能切换实际链接的 Sensor 驱动。
- 补光 PWM、IR-CUT GPIO 和产品外设宏只允许在 `main_app/peripheral/peripheral_profile_builder.*` 使用。共享驱动只读取构建后的稳定参数。
- Hi3516 软件开发工具包调用只能位于 `device/` 和 `platform/`。适配器不保存共享业务状态，也不自行按优先级选择场景、日夜、临时灯光或外设总开关（gate）。
- 禁止恢复旧的 `CLightManager`、私有 PWM/GPIO 控制或外设/日夜配置双向同步；它们会绕过共享层的优先级选择。
- 新增平台能力或调参时，先扩展共享数据模型，再由本目录的构建器（builder）和适配器转换为 Hi3516 设置；不得把平台宏泄漏到 `ipc_share`。

共享 ISP 模块的实际生效状态规则和新芯片接入要求见 [共享 ISP README](../../../../../share/ipc_share/control/business/isp/README.md)、`docs/isp模块重构/ISP新芯片平台接入指南.md` 与 `docs/isp模块重构/ISP参数映射与运行策略配置化设计.md`。
