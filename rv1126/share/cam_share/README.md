# 摄像机公共函数库模块说明

本项目为摄像机产品开发的公共函数库，旨在提供统一的工具和模块，提高代码的可维护性和复用性。以下是模块的详细说明和结构划分。

---

## 目录结构
### 1. `common` 公共模块
- **描述**：提供公共功能，如事件消息码定义和日志记录等。
- **子模块**：
  - `command_return_code`：命令返回码处理。
  - `log`：日志操作模块，可支持日志打印和控制台输出。

---

### 2. `device_system` 设备与系统管理模块
- **描述**：用于管理设备相关信息及系统资源。
- **子模块**：
  - `get_time`：时间获取模块，基于 `time` 函数获取当前日期时间。
  - `device`：设备信息获取模块。
  - `system`：系统磁盘信息获取模块。
  - `user_manage`：用户管理模块。

---

### 3. `communication` 通信模块
- **描述**：支持各种通信方式的模块，包括网络通信和协议封装。
- **子模块**：
  - `communication`：TCP 通信接口模块。
  - `communication_mqtt`：基于 MQTT 平台的通信模块。
  - `mqtt`：MQTT 封装接口。
  - `network`：网络模块，支持网络通信。
  - `udp_sdk_base`：UDP 接口模块。
  - `serial_control`：串行通信控制模块。
  - `socket`：Socket 配置相关封装。

---

### 4. `media` 媒体与编码模块
- **描述**：提供流媒体、音视频编码等相关功能。
- **子模块**：
  - `libmedia`：基于 FFmpeg 的流媒体模块。
  - `live555`：Live555 流媒体工具模块。
  - `rtspserver`：RTSP 服务端模块。
  - `enc_dec_tool`：H.264 SPS 数据解析工具。
  - `calculate`：简单计算工具模块。

---

### 5. `file_processing` 文件处理模块
- **描述**：提供文件操作与数据格式转换功能。
- **子模块**：
  - `httpdownload`：HTTP 下载功能模块。
  - `https_download`：支持 HTTPS 协议下载功能，包括 CA 证书。
  - `image`：图像处理模块，支持 Freetype 字体等。
  - `iconv`：字符编码转换模块。
  - `ini`：INI 文件读写模块。
  - `json`：JSON 数据处理模块。
  - `xml`：XML 文件读写模块。
  - `xmlconfig`：XML 数据操作模块。

---

### 6. `data` 数据库与数据处理模块
- **描述**：提供数据库操作和数据处理功能。
- **子模块**：
  - `database`：数据库操作模块。
  - `frontEncode`：字符编码互转模块，例如 GB2312 和 UTF-8 互转。
  - `md5`：MD5 算法摘要处理模块。
  - `osa_base`：数据格式优化处理模块。

---

### 7. `utils` 辅助工具模块
- **描述**：提供基础工具和封装类。
- **子模块**：
  - `cpp_base`：C++ 基础类库，目前包含封装线程操作的功能。
  - `memoryso`：堆内存管理模块。
  - `curl`：基于 libcurl 的 HTTP 封装。
  - `share_os`：C 语言封装操作模块。
  - `list`：链表操作模块。
  - `securec`：安全函数模块。

---

### 8. `src_mod` 源码模块
- **描述**：流媒体源码模块，目前包含 FFmpeg 的相关封装。

---
