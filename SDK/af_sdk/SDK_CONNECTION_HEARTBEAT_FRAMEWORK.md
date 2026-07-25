# SDK 客户端与服务端连接及心跳框架详解

---

## 一、整体架构概览

### 1.1 架构总览

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                                    SDK 完整架构图                                   │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                     │
│   ┌─────────────────────────────────────────────┐        ┌──────────────────────────┐ │
│   │              客户端 (SDK Client)             │        │        服务端 (SDK Server) │ │
│   └──────────────────────────┬──────────────────┘        └───────────┬────────────┘ │
│                              │                                      │               │
│   ┌──────────────────────────▼──────────────────┐        ┌───────────▼────────────┐ │
│   │            CUserSession (会话管理器)          │        │   CSessionManager      │ │
│   │  ┌───────────────────────────────────────┐   │        │   (单例会话管理中心)     │ │
│   │  │ • 登录认证 (Login)                    │   │        │                       │ │
│   │  │ • 心跳保活 (Heartbeat)                │   │        │  ┌─────────────────┐   │ │
│   │  │ • 断线重连 (Reconnect)                │   │        │  │ 管理所有Session │   │ │
│   │  │ • Session生命周期管理                 │   │        │  │ 超时清理线程    │   │ │
│   │  └───────────────────────────────────────┘   │        │  └────────┬────────┘   │ │
│   └──────────────────┬───────────────────────────┘        │           │            │ │
│                      │                                    │    ┌───────▼───────┐    │ │
│   ┌──────────────────▼──────────────────┐                 │    │CServerSession │    │ │
│   │      CClientAlarmManager            │                 │    │  (单个客户端)  │    │ │
│   │  ┌──────────────────────────────────┐│                 │    │  ┌──────────┐ │    │ │
│   │  │ • 报警长连接建立                  ││                 │    │  │消息队列  │ │    │ │
│   │  │ • 流式数据接收                    ││                 │    │  │心跳定时器│ │    │ │
│   │  │ • 报警数据解析与回调              ││                 │    │  │活跃时间戳│ │    │ │
│   │  └──────────────────────────────────┘│                 │    │  └──────────┘ │    │ │
│   └──────────────────┬───────────────────┘                 │    └───────┬───────┘    │ │
│                      │                                    │            │             │ │
│   ┌──────────────────▼──────────────────┐                 │    ┌───────▼───────┐    │ │
│   │       CommandExecutor               │                 │    │   SdkHttpServer│    │ │
│   │  ┌──────────────────────────────────┐│                 │    │  (HTTP服务)   │    │ │
│   │  │ • HTTP GET/POST请求封装          ││                 │    │  ┌──────────┐ │    │ │
│   │  │ • 响应解析                       ││                 │    │  │路由分发  │ │    │ │
│   │  │ • 超时控制                       ││                 │    │  │请求处理  │ │    │ │
│   │  └──────────────────────────────────┘│                 │    │  └──────────┘ │    │ │
│   └──────────────────────────────────────┘                 │    └───────┬───────┘    │ │
│                                                            │            │             │ │
│                        ▲                                   │    ┌───────▼───────┐    │ │
│                        │                                   │    │  AlarmModule  │    │ │
│                        │  HTTP 请求/响应                    │    │  (报警模块)   │    │ │
│                        │  登录/心跳/命令                    │    │  ┌──────────┐ │    │ │
│                        │                                   │    │  │报警生成  │ │    │ │
│                        │                                   │    │  │消息入队  │ │    │ │
│                        │         HTTP长连接 (SSE流式)        │    │  │推送管理  │ │    │ │
│                        │         报警推送/心跳保持           │    │  └──────────┘ │    │ │
│                        └────────────────────────────────────┼───────────────────┘    │ │
│                                                            │                        │ │
└─────────────────────────────────────────────────────────────┴────────────────────────┘
```

### 1.2 架构层次说明

| 层次 | 组件 | 职责 | 通信方式 |
|------|------|------|----------|
| **应用层** | 用户代码/Demo | 业务逻辑、回调处理 | 调用SDK接口 |
| **SDK层** | UserSession/AlarmManager | 会话管理、报警监听 | 封装HTTP/TCP |
| **传输层** | HTTP Client/Server | 请求封装、响应解析 | HTTP协议 |
| **网络层** | TCP/IP | 数据传输、连接管理 | TCP协议 |

---

## 二、核心文件职责详细说明

### 2.1 客户端核心文件详解

#### 2.1.1 UserSession.h/cpp - 会话管理器

**核心职责**：
- **登录认证**：向服务端发送登录请求，获取SessionId
- **心跳保活**：定期发送心跳请求，维持会话有效性
- **断线重连**：检测连接断开，自动重试登录
- **状态管理**：维护在线状态、SessionId、重连状态

**关键成员变量**：
| 变量名 | 类型 | 说明 |
|--------|------|------|
| `sessionId_` | `std::string` | 当前会话ID，登录成功后赋值 |
| `isOnline_` | `bool` | 是否在线状态 |
| `isReconnecting_` | `bool` | 是否正在重连 |
| `heartbeatInterval_` | `int` | 心跳间隔（秒），默认30 |
| `maxRetry_` | `int` | 心跳失败最大重试次数 |
| `reconnectDelay_` | `std::atomic<int>` | 重连延迟（指数退避） |
| `cmdClient_` | `std::unique_ptr<httplib::Client>` | HTTP客户端实例 |
| `heartbeatThread_` | `std::thread` | 心跳线程 |
| `reconnectThread_` | `std::thread` | 重连线程 |

**关键方法**：
| 方法名 | 功能 |
|--------|------|
| `ConnectAndLogin()` | 执行登录流程，获取SessionId |
| `StartHeartbeat()` | 启动心跳线程 |
| `HeartbeatLoop()` | 心跳循环（定期发送KeepLive） |
| `StartReconnect()` | 启动重连流程 |
| `ReconnectLoop()` | 重连循环（指数退避重试） |
| `Stop()` | 停止所有线程，清理资源 |

#### 2.1.2 ClientAlarmManager.h/cpp - 报警监听管理器

**核心职责**：
- **建立长连接**：创建HTTP长连接，订阅报警事件
- **流式接收**：以流式方式接收服务端推送的报警数据
- **数据解析**：解析multipart/form-data格式的报警数据
- **回调触发**：将解析后的报警数据通过回调函数传递给上层

**关键成员变量**：
| 变量名 | 类型 | 说明 |
|--------|------|------|
| `sessionId_` | `std::string` | 当前会话ID |
| `host_` | `std::string` | 服务端IP地址 |
| `port_` | `int` | 服务端端口 |
| `client_` | `std::unique_ptr<httplib::Client>` | HTTP客户端（用于长连接） |
| `isRunning_` | `bool` | 是否运行中 |
| `alarmCb_` | `NET_TV_AlarmCallBack` | 报警回调函数 |
| `channelStatusCb_` | `NET_TV_ChannelStatusCallBack` | 通道状态回调函数 |

**关键方法**：
| 方法名 | 功能 |
|--------|------|
| `Start()` | 启动报警监听线程 |
| `AlarmLoop()` | 报警监听主循环 |
| `dispatch_alarm()` | 解析并分发报警数据 |
| `Stop()` | 停止监听，关闭连接 |

#### 2.1.3 CommandExecutor.h - 命令执行器

**核心职责**：
- **HTTP请求封装**：封装GET/POST请求
- **请求超时控制**：设置连接和接收超时
- **响应解析**：将HTTP响应解析为结构化数据
- **错误处理**：统一处理HTTP错误码

---

### 2.2 服务端核心文件详解

#### 2.2.1 SessionManager.h/cpp - 会话管理器（单例）

**核心职责**：
- **会话管理**：维护所有客户端会话的生命周期
- **登录处理**：处理客户端登录请求，生成SessionId
- **心跳响应**：接收客户端心跳，更新活跃时间
- **超时清理**：定期清理超时会话
- **报警推送**：管理报警推送流程

**关键成员变量**：
| 变量名 | 类型 | 说明 |
|--------|------|------|
| `m_sessions` | `std::map<std::string, std::shared_ptr<CServerSession>>` | 会话映射表 |
| `running_` | `std::atomic<bool>` | 是否运行中 |
| `cleanupThread_` | `std::thread` | 超时清理线程 |
| `mutex_` | `std::mutex` | 线程互斥锁 |

**关键方法**：
| 方法名 | 功能 |
|--------|------|
| `Login()` | 处理登录请求，创建Session |
| `Logout()` | 处理登出请求，销毁Session |
| `KeepLive()` | 处理心跳请求，更新活跃时间 |
| `HttpCommandAlarmListen()` | 处理报警监听请求，建立长连接 |
| `CleanTimeoutSessions()` | 清理超时会话 |

#### 2.2.2 ServerSession.h/cpp - 单个客户端会话

**核心职责**：
- **会话状态管理**：维护单个客户端的连接状态
- **消息队列**：管理待推送的报警消息队列
- **心跳定时器**：管理报警长连接的心跳发送
- **活跃时间戳**：记录最后活跃时间，用于超时判断

**关键成员变量**：
| 变量名 | 类型 | 说明 |
|--------|------|------|
| `m_sessionId` | `std::string` | 会话ID |
| `m_isLogined` | `std::atomic<bool>` | 是否已登录 |
| `m_isConnected` | `std::atomic<bool>` | 是否连接中 |
| `m_lastActive` | `std::chrono::steady_clock::time_point` | 最后活跃时间 |
| `m_messageQueue` | `std::queue<AlarmData>` | 报警消息队列 |
| `m_queueMutex` | `std::mutex` | 队列互斥锁 |

**关键方法**：
| 方法名 | 功能 |
|--------|------|
| `EnqueueMessage()` | 报警消息入队 |
| `DequeueMessage()` | 报警消息出队 |
| `ClearMessageQueue()` | 清空消息队列 |
| `UpdateLastActive()` | 更新最后活跃时间 |
| `IsTimeout()` | 判断是否超时 |

#### 2.2.3 SdkHttpServer.h/cpp - HTTP服务端

**核心职责**：
- **端口监听**：监听指定端口的HTTP请求
- **路由分发**：根据URL路径分发请求到对应处理函数
- **请求处理**：处理登录、心跳、命令等请求
- **响应生成**：生成HTTP响应返回给客户端

#### 2.2.4 AlarmModule.h/cpp - 报警模块

**核心职责**：
- **报警生成**：从IPC/NVR接收报警事件，生成报警数据
- **消息入队**：将报警数据加入对应Session的消息队列
- **推送管理**：管理报警推送的优先级和频率

---

## 三、连接建立流程详解

### 3.1 完整登录流程

```
客户端 (CUserSession)                              服务端 (SessionManager)
       │                                                   │
       │ ① 创建HTTP客户端                                   │
       │    cmdClient_ = new httplib::Client(host, port)    │
       ▼                                                   │
       │ ② POST /TVAPI/V1.0/Basic/Login                   │
       │    Body: {DeviceName, UserName, Password}         │
       ├───────────────────────────────────────────────────►│
       │                                                   │
       │                                          ③ 生成唯一SessionId │
       │                                          ④ 创建CServerSession│
       │                                          ⑤ 加入m_sessions映射│
       │                                                   │
       │ ⑥ 200 OK                                         │
       │    Body: {SessionId: "session_xxx", Return: 0}    │
       ├───────────────────────────────────────────────────◄│
       │                                                   │
       │ ⑦ 保存SessionId                                   │
       │    sessionId_ = "session_xxx"                     │
       │ ⑧ 设置在线状态                                    │
       │    isOnline_ = true                               │
       │ ⑨ 启动心跳线程                                    │
       │    StartHeartbeat()                               │
       │ ⑩ 启动报警监听                                    │
       │    ClientAlarmManager::Start()                    │
       ▼                                                   ▼
```

### 3.2 登录代码详解

**客户端登录实现** (`UserSession.cpp`)：
```cpp
bool CUserSession::ConnectAndLogin() {
    // 确保之前的连接已清理
    if (cmdClient_) {
        cmdClient_.reset();
    }
    
    // 创建新的HTTP客户端
    cmdClient_ = std::make_unique<httplib::Client>(host_, port_);
    cmdClient_->set_connect_timeout(5);  // 连接超时5秒
    cmdClient_->set_read_timeout(30);    // 读取超时30秒
    
    // 发送登录请求
    httplib::Params params;
    params.emplace("DeviceName", deviceName_);
    params.emplace("UserName", userName_);
    params.emplace("Password", password_);
    
    auto res = cmdClient_->Post(TVAPI_PATH_BASIC_LOGIN, params);
    
    // 检查响应
    if (!res) {
        // 网络错误：连接失败、超时等
        NSDK_LOG_ERROR("[UserSession] Login failed: network error");
        return false;
    }
    
    if (res->status != HTTP_RESP_CODE_SUCCESS) {
        // HTTP错误：401认证失败、500服务器错误等
        NSDK_LOG_ERROR("[UserSession] Login failed: HTTP %d", res->status);
        return false;
    }
    
    // 解析响应数据
    SDKConvert::to_respStruct(res->body.c_str(), stSeesionMessage);
    sessionId_ = stSeesionMessage.SeesionId;
    
    if (sessionId_.empty()) {
        NSDK_LOG_ERROR("[UserSession] Login failed: empty session ID");
        return false;
    }
    
    // 登录成功，设置状态
    isOnline_ = true;
    isReconnecting_ = false;
    reconnectDelay_ = 1;  // 重置重连延迟
    
    NSDK_LOG_INFO("[UserSession] Login success, SessionId=%s", sessionId_.c_str());
    return true;
}
```

**服务端登录处理** (`SessionManager.cpp`)：
```cpp
bool CSessionManager::Login(std::string& OutSessionId, 
                            const std::string& clientIP,
                            const std::string& deviceName) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 生成唯一SessionId（格式：session_<时间戳>_<随机数>）
    OutSessionId = "session_" + std::to_string(std::time(nullptr)) + 
                   "_" + std::to_string(rand() % 10000);
    
    // 创建新的会话对象
    auto newSession = std::make_shared<CServerSession>(OutSessionId);
    newSession->SetClientIP(clientIP);
    newSession->SetDeviceName(deviceName);
    newSession->SetLogined(true);
    newSession->SetConnected(true);
    newSession->UpdateLastActive();  // 记录登录时间
    
    // 添加到会话管理
    m_sessions[OutSessionId] = newSession;
    
    NSDK_LOG_INFO("[SessionManager] New session created: %s, IP: %s, Total: %zu", 
                  OutSessionId.c_str(), clientIP.c_str(), m_sessions.size());
    
    return true;
}
```

---

## 四、心跳保活机制详解

### 4.1 客户端心跳机制

客户端采用**双心跳机制**，确保连接可靠性：

#### 4.1.1 HTTP心跳（主动心跳）

**作用**：通过定期发送HTTP请求确认服务端可达性

**流程**：
```
客户端                              服务端
  │                                   │
  │ ① 等待心跳间隔 (默认30秒)           │
  │                                   │
  │ ② GET /TVAPI/V1.0/Basic/KeepLive │
  │    ?session_id=session_xxx        │
  ├──────────────────────────────────►│
  │                                   │
  │ ③ 更新lastActive时间戳            │
  │                                   │
  │ ④ 200 OK {Return: 0}             │
  ├──────────────────────────────────◄│
  │                                   │
  │ ⑤ 重置失败计数                    │
  │    failCount = 0                  │
  └──────────────────────────────────┘
```

**心跳失败处理**：
```
心跳失败 → failCount++ → 判断是否达到maxRetry_
                           │
              ┌────────────┴────────────┐
              ▼                         ▼
        未达到maxRetry_            达到maxRetry_
              │                         │
              ▼                         ▼
        继续等待下次心跳           启动重连线程
                                   ReconnectLoop()
```

**代码实现** (`UserSession.cpp`)：
```cpp
void CUserSession::HeartbeatLoop() {
    int failCount = 0;  // 心跳失败计数
    const int maxRetry = 3;  // 最大重试次数
    
    while (isRunning_) {
        // 心跳间隔等待（每秒检查一次是否停止）
        for (int i = 0; i < heartbeatInterval_ && isRunning_; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        if (!isRunning_) break;
        
        // 检查是否正在重连（重连期间不发送心跳）
        if (isReconnecting_) {
            continue;
        }
        
        // 发送心跳请求
        std::string url = TVAPI_PATH_BASIC_KEEPLIVE + "?session_id=" + sessionId_;
        auto res = cmdClient_->Get(url.c_str());
        
        if (res && res->status == HTTP_RESP_CODE_SUCCESS) {
            // 心跳成功
            failCount = 0;
            isOnline_ = true;
            NSDK_LOG_DEBUG("[UserSession] Heartbeat success, SessionId=%s", sessionId_.c_str());
        } else {
            // 心跳失败
            failCount++;
            isOnline_ = false;
            NSDK_LOG_WARN("[UserSession] Heartbeat failed (%d/%d), SessionId=%s", 
                         failCount, maxRetry, sessionId_.c_str());
            
            if (failCount >= maxRetry) {
                // 达到最大重试次数，启动重连
                NSDK_LOG_ERROR("[UserSession] Heartbeat failed %d times, starting reconnect", maxRetry);
                StartReconnect();
                break;  // 退出心跳循环，等待重连
            }
        }
    }
}
```

#### 4.1.2 SSE长连接心跳（被动心跳）

**作用**：服务端通过长连接定期推送心跳包，保持TCP连接活跃

**特点**：
- 服务端每30秒发送一次心跳
- 客户端不需要主动发送请求
- 主要用于保持TCP连接不被中间设备断开

### 4.2 服务端心跳处理

#### 4.2.1 HTTP心跳响应

**处理流程** (`SessionManager.cpp`)：
```cpp
bool CSessionManager::KeepLive(const std::string& SessionId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = m_sessions.find(SessionId);
    if (it == m_sessions.end()) {
        NSDK_LOG_WARN("[SessionManager] KeepLive failed: session not found, SessionId=%s", 
                      SessionId.c_str());
        return false;
    }
    
    // 更新最后活跃时间
    it->second->UpdateLastActive();
    NSDK_LOG_DEBUG("[SessionManager] KeepLive success, SessionId=%s", SessionId.c_str());
    
    return true;
}
```

#### 4.2.2 报警长连接心跳

**处理流程** (`SessionManager.cpp`)：
```cpp
void CSessionManager::HttpCommandAlarmListen(const httplib::Request& req, 
                                             httplib::Response& res) {
    std::string SessionId = req.get_param_value("session_id");
    
    // ... 验证Session ...
    
    std::string boundary = "boundary_" + std::to_string(std::time(nullptr));
    
    // 设置响应为流式输出（multipart/form-data）
    res.set_content_provider(
        "multipart/form-data; boundary=" + boundary,
        // 数据生成回调函数（在单独线程中执行）
        [this, SessionId, boundary](size_t offset, httplib::DataSink& sink) -> bool {
            // 检查Session是否存在且已登录
            auto sess = GetSession(SessionId);
            if (!sess || !sess->IsLogined()) {
                NSDK_LOG_WARN("[SessionManager] Alarm listen stopped: session invalid");
                return false;  // 返回false表示结束流式输出
            }
            
            // 更新活跃时间（每次输出数据时都会更新）
            sess->UpdateLastActive();
            
            // 获取当前时间
            auto now = std::chrono::steady_clock::now();
            
            // 检查是否需要发送心跳（每30秒一次）
            if (std::chrono::duration_cast<std::chrono::seconds>(now - sess->GetLastHeartbeat()).count() >= 30) {
                std::string heartbeat = "--" + boundary + "\r\n"
                                      "Content-Disposition: form-data; name=\"heartbeat\"\r\n"
                                      "Content-Type: application/json\r\n\r\n"
                                      "{\"type\":\"heartbeat\",\"timestamp\":" + 
                                      std::to_string(std::time(nullptr)) + "}\r\n";
                sink.write(heartbeat.data(), heartbeat.size());
                sess->UpdateLastHeartbeat();  // 更新心跳时间
                return true;  // 继续保持连接
            }
            
            // 尝试从队列中获取报警消息
            CServerSession::AlarmData msg;
            if (sess->DequeueMessage(msg)) {
                // 发送报警数据
                std::stringstream ss;
                ss << "--" << boundary << "\r\n";
                ss << "Content-Disposition: form-data; name=\"alarm\"\r\n";
                ss << "Content-Type: application/json\r\n\r\n";
                ss << msg.json << "\r\n";
                
                std::string data = ss.str();
                sink.write(data.data(), data.size());
                
                // 如果有附件（如图片），继续发送附件
                for (const auto& attachment : msg.attachments) {
                    std::stringstream attSS;
                    attSS << "--" << boundary << "\r\n";
                    attSS << "Content-Disposition: form-data; name=\"attachment\"; filename=\"" 
                          << attachment.filename << "\"\r\n";
                    attSS << "Content-Type: " << attachment.contentType << "\r\n\r\n";
                    
                    sink.write(attSS.str().data(), attSS.str().size());
                    sink.write(attachment.data.data(), attachment.data.size());
                    sink.write("\r\n", 2);
                }
                
                NSDK_LOG_DEBUG("[SessionManager] Alarm sent, SessionId=%s", SessionId.c_str());
                return true;
            }
            
            // 没有数据，短暂休眠后继续
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            return true;
        }
    );
    
    NSDK_LOG_INFO("[SessionManager] Alarm listen started: SessionId=%s", SessionId.c_str());
}
```

---

## 五、报警推送机制详解

### 5.1 报警推送完整流程

```
IPC/NVR                    AlarmModule              SessionManager          CServerSession             客户端
   │                           │                         │                         │                        │
   │ ① 报警事件发生             │                         │                         │                        │
   │  (移动侦测/越界等)          │                         │                         │                        │
   ├───────────────────────────►│                         │                         │                        │
   │                           │ ② 生成报警JSON数据       │                         │                        │
   │                           │                         │                         │                        │
   │                           │ ③ 调用PushAlarmToSession │                         │                        │
   │                           ├─────────────────────────►│                         │                        │
   │                           │                         │ ④ 查找目标Session         │                        │
   │                           │                         │                         │                        │
   │                           │                         │ ⑤ 调用EnqueueMessage      │                        │
   │                           │                         ├──────────────────────────►│                        │
   │                           │                         │                         │ ⑥ 消息入队              │
   │                           │                         │                         │                        │
   │                           │                         │ ⑦ HTTP长连接检测消息      │                        │
   │                           │                         │◄───────────────────────────│                        │
   │                           │                         │                         │                        │
   │                           │                         │ ⑧ 通过流式响应发送        │                        │
   │                           │                         │─────────────────────────────────────────────────────►│
   │                           │                         │                         │                        │ ⑨ 解析multipart数据
   │                           │                         │                         │                        │ ⑩ 触发回调函数
```

### 5.2 报警数据结构

**服务端报警数据** (`ServerSession.h`)：
```cpp
struct Attachment {
    std::string filename;       // 附件文件名
    std::string contentType;    // MIME类型（如image/jpeg）
    std::vector<char> data;     // 附件二进制数据
};

struct AlarmData {
    std::string json;                      // 报警主体JSON数据
    std::vector<Attachment> attachments;   // 附件列表（图片等）
};
```

**客户端报警回调**：
```cpp
// 报警回调函数类型
typedef void (STDCALL *NET_TV_AlarmCallBack)(NET_TV_ALARM_INFO_S *pAlarmInfo, LPVOID lpUserData);

// 通道状态回调函数类型
typedef void (STDCALL *NET_TV_ChannelStatusCallBack)(NET_TV_CHANNEL_INFO_S *pChannelInfo, LPVOID lpUserData);
```

### 5.3 客户端报警接收

**报警监听循环** (`ClientAlarmManager.cpp`)：
```cpp
void CClientAlarmManager::AlarmLoop() {
    while (isRunning_) {
        // 创建新的HTTP客户端（每次重连都创建新实例）
        client_ = std::make_unique<httplib::Client>(host_, port_);
        client_->set_read_timeout(300);  // 5分钟读取超时
        client_->set_keep_alive(true);   // 启用长连接
        client_->set_connection_timeout(10);  // 连接超时10秒
        
        std::string url = TVAPI_PATH_ALARMEVENT_LISTEN + "?session_id=" + sessionId_;
        
        NSDK_LOG_INFO("[ClientAlarmManager] Connecting to alarm listen: %s:%d%s", 
                      host_.c_str(), port_, url.c_str());
        
        // 发起GET请求，注册数据回调
        auto res = client_->Get(url.c_str(),
            // 响应头回调（可选）
            [&](const httplib::Response& response) {
                NSDK_LOG_DEBUG("[ClientAlarmManager] Response headers received");
            },
            // 数据块回调（流式接收）
            [&](const char* data, size_t len) {
                // 将收到的数据追加到缓冲区
                if (len > 0) {
                    pendingBuffer.append(data, len);
                    // 尝试解析缓冲区中的完整消息
                    parseBuffer();
                }
                return true;  // 返回true继续接收
            }
        );
        
        // 请求结束或失败
        if (!res) {
            NSDK_LOG_WARN("[ClientAlarmManager] Alarm listen disconnected: %s", 
                         httplib::to_string(res.error()).c_str());
        } else if (res->status != 200) {
            NSDK_LOG_ERROR("[ClientAlarmManager] Alarm listen failed: HTTP %d", res->status);
            if (res->status == 401) {
                // Session过期，需要重新登录
                NSDK_LOG_ERROR("[ClientAlarmManager] Session expired, need re-login");
                if (loginCb_) {
                    loginCb_();  // 通知上层重新登录
                }
            }
        }
        
        // 清理缓冲区
        pendingBuffer.clear();
        
        // 如果不是主动停止，等待1秒后重连
        if (isRunning_) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    NSDK_LOG_INFO("[ClientAlarmManager] Alarm loop stopped");
}

// 解析缓冲区中的multipart数据
void CClientAlarmManager::parseBuffer() {
    // 查找boundary分隔符
    // ... 解析multipart格式 ...
    
    // 提取JSON数据
    if (isJsonData) {
        dispatch_alarm(jsonString);
    }
    
    // 提取附件数据
    if (isAttachment) {
        saveAttachment(filename, data);
    }
}

// 分发报警数据
void CClientAlarmManager::dispatch_alarm(const std::string& jsonBody) {
    // 解析JSON
    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(jsonBody, root)) {
        NSDK_LOG_WARN("[ClientAlarmManager] Failed to parse alarm JSON");
        return;
    }
    
    // 获取命令类型
    long long command = root["Command"].asInt64();
    
    // 根据命令类型分发
    switch (command) {
        case NET_TV_NOTIFY_ALARM: {
            // 普通报警
            NET_TV_ALARM_INFO_S alarmInfo = {0};
            SDKConvert::deal(root["AlarmInfo"], alarmInfo);
            
            if (alarmCb_) {
                alarmCb_(&alarmInfo, alarmUserData_);
            }
            break;
        }
        case NET_TV_NOTIFY_CHANNEL_STATUS: {
            // 通道状态变更
            NET_TV_CHANNEL_INFO_S channelInfo = {0};
            SDKConvert::deal(root["ChannelInfo"], channelInfo);
            
            if (channelStatusCb_) {
                channelStatusCb_(&channelInfo, channelStatusUserData_);
            }
            break;
        }
        // ... 其他命令类型 ...
    }
}
```

---

## 六、重连机制详解

### 6.1 客户端重连流程

**触发条件**：
1. HTTP心跳失败次数达到 `maxRetry_`（默认3次）
2. 报警监听连接断开且非主动停止
3. 收到HTTP 401错误（Session过期）

**重连策略**：指数退避（Exponential Backoff）
- 第一次重试：1秒后
- 第二次重试：2秒后
- 第三次重试：4秒后
- ...
- 最大延迟：60秒

**流程**：
```
连接断开/心跳失败 → 启动ReconnectLoop线程
                       │
                       ▼
              等待指数退避延迟
                       │
                       ▼
              创建新HTTP客户端
                       │
                       ▼
              执行ConnectAndLogin()
                       │
              ┌────────┴────────┐
              ▼                 ▼
         登录成功            登录失败
              │                 │
              ▼                 ▼
         重置延迟为1       延迟翻倍（最大60秒）
         启动心跳线程         继续循环
         启动报警监听
              │
              ▼
         重连完成
```

**代码实现** (`UserSession.cpp`)：
```cpp
void CUserSession::StartReconnect() {
    if (isReconnecting_) {
        return;  // 已经在重连中
    }
    
    isReconnecting_ = true;
    isOnline_ = false;
    
    // 如果心跳线程还在运行，停止它
    if (heartbeatThread_.joinable()) {
        heartbeatThread_.join();
    }
    
    // 启动重连线程
    if (!reconnectThread_.joinable()) {
        reconnectThread_ = std::thread(&CUserSession::ReconnectLoop, this);
    }
}

void CUserSession::ReconnectLoop() {
    NSDK_LOG_INFO("[UserSession] Reconnect loop started");
    
    while (isReconnecting_ && isRunning_) {
        // 获取当前重连延迟
        int delay = reconnectDelay_.load();
        NSDK_LOG_INFO("[UserSession] Reconnect attempt in %d seconds...", delay);
        
        // 等待延迟时间
        for (int i = 0; i < delay && isReconnecting_ && isRunning_; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        if (!isReconnecting_ || !isRunning_) {
            break;
        }
        
        NSDK_LOG_INFO("[UserSession] Attempting to reconnect...");
        
        // 创建新的HTTP客户端（避免使用旧的连接状态）
        cmdClient_ = std::make_unique<httplib::Client>(host_, port_);
        cmdClient_->set_connect_timeout(5);
        cmdClient_->set_read_timeout(30);
        
        // 尝试登录
        if (ConnectAndLogin()) {
            // 重连成功
            NSDK_LOG_INFO("[UserSession] Reconnect success!");
            
            // 启动心跳
            StartHeartbeat();
            
            // 通知上层重连成功
            if (reconnectCb_) {
                reconnectCb_(true);
            }
            
            break;  // 退出重连循环
        } else {
            // 重连失败
            NSDK_LOG_WARN("[UserSession] Reconnect failed");
            
            // 指数退避：延迟翻倍，最大60秒
            int newDelay = std::min(delay * 2, 60);
            reconnectDelay_.store(newDelay);
            
            // 通知上层重连失败
            if (reconnectCb_) {
                reconnectCb_(false);
            }
        }
    }
    
    NSDK_LOG_INFO("[UserSession] Reconnect loop ended");
}
```

### 6.2 服务端会话清理

**超时判断逻辑** (`ServerSession.cpp`)：
```cpp
bool CServerSession::IsTimeout(int timeoutSeconds) const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastActive);
    return elapsed.count() >= timeoutSeconds;
}
```

**超时清理线程** (`SessionManager.cpp`)：
```cpp
void CSessionManager::StartCleanupThread() {
    cleanupThread_ = std::thread(&CSessionManager::CleanupLoop, this);
}

void CSessionManager::CleanupLoop() {
    NSDK_LOG_INFO("[SessionManager] Cleanup loop started");
    
    while (running_) {
        // 每分钟检查一次超时会话
        CleanTimeoutSessions();
        
        // 等待60秒
        for (int i = 0; i < 60 && running_; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    NSDK_LOG_INFO("[SessionManager] Cleanup loop ended");
}

void CSessionManager::CleanTimeoutSessions() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    int cleanedCount = 0;
    for (auto it = m_sessions.begin(); it != m_sessions.end();) {
        if (it->second->IsTimeout(SESSION_TIMEOUT_SEC)) {
            NSDK_LOG_INFO("[SessionManager] Session timeout, cleaning: %s", 
                          it->first.c_str());
            it = m_sessions.erase(it);
            cleanedCount++;
        } else {
            ++it;
        }
    }
    
    if (cleanedCount > 0) {
        NSDK_LOG_INFO("[SessionManager] Cleaned %d timeout sessions, remaining: %zu", 
                      cleanedCount, m_sessions.size());
    }
}
```

---

## 七、关键配置参数详解

### 7.1 客户端配置参数

| 参数名 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| `heartbeatInterval_` | `int` | 30秒 | HTTP心跳间隔，建议值：10-60秒 |
| `maxRetry_` | `int` | 3次 | 心跳失败最大重试次数，超过后启动重连 |
| `connectTimeout` | `int` | 5秒 | HTTP连接超时时间 |
| `readTimeout` | `int` | 30秒 | HTTP读取超时时间 |
| `alarmReadTimeout` | `int` | 300秒 | 报警长连接读取超时（5分钟） |
| `maxReconnectDelay_` | `int` | 60秒 | 重连最大延迟（指数退避上限） |

### 7.2 服务端配置参数

| 参数名 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| `SESSION_TIMEOUT_SEC` | `int` | 300秒 | 会话超时时间，超时后自动清理 |
| `HEARTBEAT_INTERVAL_SEC` | `int` | 30秒 | 报警长连接心跳间隔 |
| `CLEANUP_INTERVAL_SEC` | `int` | 60秒 | 超时清理线程检查间隔 |
| `MAX_QUEUE_SIZE` | `int` | 1000 | 单个Session消息队列最大长度 |

### 7.3 配置建议

**生产环境建议配置**：
| 参数 | 建议值 | 说明 |
|------|--------|------|
| 客户端心跳间隔 | 20-30秒 | 平衡网络开销和响应速度 |
| 服务端会话超时 | 300-600秒 | 考虑网络不稳定情况 |
| 报警长连接超时 | 300-600秒 | 避免频繁重连 |
| 重连最大延迟 | 60秒 | 避免重试风暴 |

---

## 八、故障排查指南

### 8.1 常见问题定位

| 现象 | 可能原因 | 排查方向 |
|------|---------|---------|
| **登录失败** | 网络不通、账号密码错误、服务端未启动 | 检查网络连通性、账号配置、服务端日志 |
| **心跳失败** | 网络断开、服务端宕机、Session过期 | 检查网络、服务端状态、Session有效性 |
| **报警丢失** | 队列溢出、连接断开、IPC未推送 | 检查队列状态、连接状态、IPC日志 |
| **401错误** | Session过期、重复登录 | 检查登录状态、是否有重复登录 |
| **连接频繁断开** | 网络超时、防火墙限制、keep-alive配置 | 检查中间设备、调整超时配置 |
| **重连不成功** | 服务端未恢复、网络持续故障 | 检查服务端状态、网络稳定性 |

### 8.2 日志关键字速查

**客户端日志关键字**：
| 关键字 | 含义 | 处理建议 |
|--------|------|----------|
| `Login failed` | 登录失败 | 检查网络、账号、服务端 |
| `Heartbeat failed` | 心跳失败 | 检查网络、服务端状态 |
| `Reconnect attempt` | 正在重连 | 等待或检查服务端 |
| `Alarm listen disconnected` | 报警监听断开 | 检查网络、Session状态 |
| `Session expired` | Session过期 | 需要重新登录 |

**服务端日志关键字**：
| 关键字 | 含义 | 处理建议 |
|--------|------|----------|
| `New session created` | 新会话创建 | 正常日志 |
| `Session timeout` | 会话超时 | 检查客户端是否正常退出 |
| `Alarm listen started` | 报警监听开始 | 正常日志 |
| `Alarm listen stopped` | 报警监听停止 | 检查客户端状态 |
| `KeepLive failed` | 心跳处理失败 | 检查Session是否存在 |

### 8.3 排查步骤示例

**问题**：客户端无法接收报警

**排查步骤**：
1. **检查网络连通性**：
   - `ping <服务端IP>` 检查网络可达性
   - `telnet <服务端IP> <端口>` 检查端口是否开放

2. **检查登录状态**：
   - 查看客户端日志是否有 `Login success`
   - 检查SessionId是否正确获取

3. **检查报警监听**：
   - 查看客户端日志是否有 `Alarm Subscribe Start`
   - 查看服务端日志是否有对应Session的报警监听记录

4. **检查心跳状态**：
   - 查看客户端日志是否有 `Heartbeat success`
   - 检查服务端是否收到心跳请求

5. **检查IPC推送**：
   - 检查IPC设备是否在线
   - 查看IPC端是否有报警事件生成
   - 查看服务端AlarmModule是否收到报警

6. **检查消息队列**：
   - 检查ServerSession的消息队列是否有消息
   - 检查队列是否溢出

---

## 九、总结

### 9.1 架构特点

1. **多层保活机制**：
   - HTTP心跳（客户端主动）：确认服务端可达性
   - SSE长连接心跳（服务端主动）：保持TCP连接活跃

2. **可靠重连策略**：
   - 指数退避：避免重试风暴
   - 状态重置：重连成功后重置所有状态

3. **线程安全设计**：
   - 使用互斥锁保护共享资源
   - 使用原子变量管理状态标志

4. **优雅的资源管理**：
   - 智能指针管理动态资源
   - 线程析构时正确join

### 9.2 数据流向

```
登录请求:    Client → HttpServer → SessionManager → 创建Session → 返回SessionId

心跳请求:    Client → HttpServer → SessionManager → 更新LastActive → 返回OK

报警推送:    IPC → AlarmModule → SessionManager → ServerSession(队列) → Http长连接 → Client → 回调

重连流程:    Client(检测失败) → ReconnectLoop → 登录 → 启动心跳 → 启动报警监听
```

### 9.3 关键设计模式

| 模式 | 应用场景 |
|------|----------|
| **单例模式** | SessionManager（全局唯一会话管理） |
| **观察者模式** | 报警回调（ClientAlarmManager注册回调） |
| **生产者-消费者模式** | 报警消息队列（AlarmModule生产，HttpCommandAlarmListen消费） |
| **策略模式** | 不同报警类型的分发处理 |

---

**文件位置**：`sdk/SDK_CONNECTION_HEARTBEAT_FRAMEWORK.md`  
**生成时间**：2026-05-25  
**版本**：v2.0（详细版）