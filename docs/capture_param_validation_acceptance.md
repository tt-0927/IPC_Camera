# IPC 抓图参数范围校验：部署与验收

日期：2026-09-08

## 修改范围

只修改 RV1126 IPC 抓图参数校验与错误返回，不修改 SDK 命令路由、JSON 转换、公共接口或结构体。
不修改视频分辨率切换、JPEG 实际尺寸、图片格式转换和质量应用。

- IPC 的 cb_set_capture_param_info 检查 SDK 传入的两组结构体参数，越界直接返回 NET_E_INVALID_PARAM（102），不执行设置任务。
- IPC 抓图任务在保存前校验 Timing、Event 两组配置；非法输入返回 ERR_WEB_PARAM（-300），不保存、不刷新。
- IPC 回调读取任务业务 Return，将 -300 映射为 102；其他任务失败返回设置配置失败。
- SDK 沿用既有回调结果封装，返回 return=102、message="Invalid parameter"。

## 参数范围

| 字段 | 允许值 |
| --- | --- |
| Enable | IPC 结构体为 FALSE/TRUE；IPC JSON 为布尔值或整数 0、1 |
| Width、Height | 整数 1～8192 |
| PictureFormat | 整数 0～1 |
| ImageQuality | 整数 0～2 |
| Number | 整数 1～120 |
| TimeUnit | 整数 0～4，依次表示毫秒、秒、分钟、小时、天 |
| Interval | 整数；按时间单位上限依次为 86400000、86400、1440、24、365，下限均为 1 |

宽高范围沿用既有接口定义，不代表设备支持范围内任意尺寸的实际出图。
两组配置应完整传入，关闭状态下也进行参数检查。

## 仅 IPC 校验的边界

SDK 先把原始 JSON 转为结构体，然后调用 IPC。因此 IPC 能校验收到的数值，但不能恢复 SDK 转换前的信息。
例如 Width=1920.5 可能已被 SDK 转成整数 1920，启用字段也可能已被归一化为布尔值；此类原始格式错误不承诺通过 SDK 接口全部拦截。
直接进入 IPC 抓图任务的 JSON 仍会拒绝小数、字符串、缺失字段和超大数值。

## 编译与部署

本次撤销了 SjclDomain.cpp 的改动，删除了 SDK 新增的 capture_param_validation.h。无需为本次修改重新编译 SDK 服务端库。

RV1126 更新以下三个文件：

- control/business/capture/capture_param_validation.h（新增）
- control/task/sub_task/capture_task.cpp（修改）
- protocols/tvsdk/src/callbacks/tvsdk_callbacks.cpp（修改）

实际编译目录为 rv1126/share/ipc_share；rv1126/ipc_share 是 Git 跟踪镜像。当前本地两处一致，编译机应更新其实际 share/ipc_share 目录，不能只拷贝镜像。
沿用已匹配的 SDK 库，使用现有目标型号和 RV1126B 工具链重新编译、部署 stream。
本地 Windows 未执行目标机交叉编译或设备部署，不要将宿主机测试程序部署到设备。

## Apifox 验收

先通过命令 210 读取并备份完整原配置。
复用已登录的设置请求 /TVAPI/V1.0/Device/SetDevConfig，command=211，session_id 和 channel 沿用已验证有效的值。

传给 SDK 配置接口的业务 JSON 示例（保持现有请求封装）：

~~~json
{
  "CaptureTimingConfig": {
    "Enable": false,
    "PictureFormat": 0,
    "Width": 1920,
    "Height": 1080,
    "ImageQuality": 1,
    "Interval": 2000,
    "TimeUnit": 0,
    "Number": 20
  },
  "CaptureEventConfig": {
    "Enable": false,
    "PictureFormat": 0,
    "Width": 1920,
    "Height": 1080,
    "ImageQuality": 1,
    "Interval": 2000,
    "TimeUnit": 0,
    "Number": 20
  }
}
~~~

正常请求预期 return=0；每次只改动一个字段，分别对两组配置执行以下测试：

| 输入 | 预期 |
| --- | --- |
| Number=121 或 Number=0 | return=102 |
| Width=8193 或 Height=0 | return=102 |
| ImageQuality=3、PictureFormat=2、TimeUnit=5 | return=102 |
| TimeUnit=1 且 Interval=86401 | return=102 |
| TimeUnit=3 且 Interval=25 | return=102 |
| TimeUnit=4 且 Interval=366 | return=102 |
| Number=1 或 120，其他字段合法 | 参数校验通过 |

HTTP 200 不代表业务设置成功，必须检查 return。
每次非法 SET 后调用 GET 命令 210，确认两组配置均未变化；测试结束恢复备份配置。
IPC 直达任务使用 Timing/Event 和嵌套 TimeInterval，内部参数错误码为 -300，不要与 SDK 的 102 混淆。

## 本次验证

- 当前 IPC JSON 校验：176 项通过。
- 实际抓图任务函数提取测试：176 项通过；配置保存等外部依赖使用替身，确认非法参数不解析、不保存、不刷新。
- 当前抓图回调函数提取测试：8 项通过；任务执行等外部依赖使用替身，覆盖参数错误和执行失败返回。
- 当前 IPC 结构体校验函数：40 项边界测试通过。
- 共 400 项重新编译运行的宿主机测试，不等同于设备端端到端测试。
- 待完成：编译机交叉编译、部署与设备端 SET/GET 验收。
