# upgrade 升级程序（hi3516）

独立升级守护进程：接收网页 CGI / ONVIF HTTP / MQTT 云端三个入口转发的升级请求
（UDS 收 `AC_DO_UPGRADE`），流式校验并安装升级包，完成后重启。

## 目录结构

- `main.cpp`：进程入口（信号、日志、时区初始化）
- `manage/upgrade_logic.cpp`：升级主流程（包头解析、型号匹配检查、流式 MD5、
  流式解压、清单校验与差集清理）
- `manage/manifest_manage.cpp`：升级文件清单（解析与逐文件 sha256 校验，fail-closed）；清理类机制暂缓（见规范 §6）
- `manage/data_manage.cpp`：升级状态（内存态）
- `common/`：升级通用定义；SHA-256 实现下沉在 `share/ipc_share/encrypt/sha256/`
  （CMake 经 `${IPC_SHARE_PATH}/encrypt/sha256/sha256.cmake` 引入，与 md5 同域）
- `common/upgrade_define.h`：包头结构、清单路径与限额常量
- `communication/`：UDS 服务端

## 升级包格式与清单规范

见 `docs/upgrade/升级包清单格式规范.md`（外层格式红线、适用型号、manifest 格式、
清理规则、运维节奏）。改动升级流程前必读，尤其是：

- 包头 `version[0]` 恒为 NUL（ONVIF 兼容承重字节），适用型号从 `[1]` 起；
- 清单基线必须在解压前读取；
- 清理只删旧基线内、受控目录下的常规文件。
