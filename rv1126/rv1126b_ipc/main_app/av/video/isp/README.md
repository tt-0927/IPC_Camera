# RV1126B ISP 业务适配

本目录将 RV1126B 的 RK AIQ 实现接入 `share/ipc_share/control/business/isp`。共享仓库负责配置事务、日夜模式、时间段、自动过滤、场景调度、补光仲裁与执行顺序；本业务仓仅保留 RK AIQ 与板级硬件差异。

## 目录职责

| 目录/文件 | 职责 |
| --- | --- |
| `bootstrap/` | 创建能力画像、四个端口适配器和共享 `CIspBusinessService`，并管理完整生命周期。 |
| `capability/` | 将 RV1126B 的功能范围声明为共享 ISP 可消费的能力画像。 |
| `platform/` | 实现参数、场景、自动日夜观测三个 RK 平台端口；外设端口复用共享 `CIspPeripheralController`。 |
| `isp_control.*` | 保留基础 RK AIQ 参数下发；不再处理日夜模式、定时、过滤或补光。 |
| `isp_scene.*` | 将共享的内部运行场景真实映射为 RK IQ 场景。 |
| `isp_dayNight.*` | 仅封装 SmartIR 环境观测，观测结果回调给共享模式控制器。 |

## 已确认的产品约束

`TV-3881T` 与 `TV-3882TI` 的 ISP 补光硬件一致，均为白光-only：

| 项目 | 取值 |
| --- | --- |
| 白光 PWM | `pwmchip1/pwm0`（控制器 `1`、通道 `0`） |
| 红外 PWM 板级预留 | 控制器 `2`、通道 `0` |
| 红外灯 / IR-CUT | 不支持；不会创建或写入红外、IR-CUT 输出 |
| 共享能力宏 | `CAP_LIGHT_WHITE_ONLY=1`、`CAP_ISP_IR_SWITCH=0` |

虽然板级资料记录了红外 PWM 预留编号，当前产品能力画像刻意不输出该通道，避免任何配置路径误驱动未支持硬件。

## IQ 映射

RV1126B 的夜间画面始终采用全彩 IQ，不存在独立低照/黑白 IQ。共享运行场景与 RK 场景的映射固定如下：

| 共享运行场景 | RK IQ 场景 | 灯光目标 |
| --- | --- | --- |
| `DAY` | `normal/day` | 关闭白光 |
| `NIGHT_WHITE` | `normal/day` | 按配置开启白光 |
| `NIGHT_LIGHT_OFF` | `normal/day` | 关闭白光 |

因此即使用户在夜间关闭白光，系统仍停留在 `normal/day` 全彩 IQ；禁止切换至 `normal/night`。

## 生命周期与调用链

`CStreamVideo::init()` 的顺序必须保持为：

```text
CIspControl::init()                 # 创建并启动 RK AIQ
CRv1126bIspRuntimeBootstrap::init() # 初始化白光驱动并注册/启动共享 ISP 服务
initStream()
```

退出或初始化失败时按反序执行，确保 SmartIR 回调、共享 reconciler 和白光闪烁 worker 不会访问已释放的 RK AIQ 上下文。

```text
共享 ISP 服务停止并注销
白光驱动停止并关灯
CIspControl::deinit()
```

业务代码不得重新引入直接的 `CPwmCtrl`、`CLightingController` 或 `CIspControl::set_dayNight_attr()` 调用；所有日夜与补光配置须经 `CIspManage` 的共享事务入口。
