# BLE 配网通信协议 V1.0

本文面向 Android、iOS 和蓝牙调试工具开发人员，描述 App 与设备之间的 BLE 配网协议。

> [!IMPORTANT]
> `0x0101`～`0x0202` 是安全业务指令。App 必须先完成二维码密钥认证，再用 AES-256-GCM 构造安全 Payload。不能把 Wi-Fi 密码、平台密码或业务 JSON 作为明文直接写入特征值。

## 1. App 对接速览

### 1.1 完整配网顺序

```text
扫描指定 Service UUID
        ↓
使用二维码中的型号、序列号匹配目标设备
        ↓
连接设备并发现 Service、Command、Response
        ↓
订阅 Response Notify
        ↓
发送 0x0001，确认协议版本和安全能力
        ↓
0x0010 开始认证：发送 appNonce
        ↓
收到 sessionId 和 deviceNonce
        ↓
0x0011 提交 HMAC 证明
        ↓
认证成功并派生 sessionKey
        ↓
0x0105 扫描 Wi-Fi（可选）
        ↓
0x0101 加密发送 Wi-Fi 凭据
        ↓
0x0102 发起 Wi-Fi 连接
        ↓
等待最终响应，或使用 0x0103 查询到 state=3
        ↓
0x0201 加密发送平台参数并连接平台
        ↓
等待最终响应，或使用 0x0202 查询到 state=2
        ↓
设备约 3 秒后关闭 BLE，配网完成
```

### 1.2 开发时必须记住的规则

1. GATT 中传输的是原始二进制字节，不是带 `0x` 前缀的字符串，也不是十六进制文本。
2. 所有多字节整数均使用大端序（Network Byte Order）。
3. App 必须先订阅 Response Notify，再发送指令。
4. `sequence` 用于匹配请求与响应；安全 Payload 中的 `counter` 用于防重放，两者不是同一个字段。
5. Wi-Fi 连接、Wi-Fi 扫描和平台连接会返回两次：先返回“已受理”，再返回“最终结果”。
6. 第一条空 Data 的 `STATUS_OK` 不表示连接或扫描已经完成。
7. 单帧可能超过 BLE 单次收发长度：App 写入请求时必须分片；设备 Notify 响应也可能分片，App 必须先组帧再解析。
8. 每次重新认证后 `counter` 从 1 开始；只有设备成功解密的请求才消耗该 counter。
9. 响应帧当前不加密，但不会返回 Wi-Fi 密码、平台密码或会话密钥。

## 2. GATT 定义

| 项目 | UUID | App 操作 |
|---|---|---|
| Provisioning Service | `7A1E0001-7D8B-4F31-9E45-545650524F56` | 扫描过滤、服务发现 |
| Command Characteristic | `7A1E0002-7D8B-4F31-9E45-545650524F56` | Write / Write Without Response |
| Response Characteristic | `7A1E0003-7D8B-4F31-9E45-545650524F56` | Read / Notify |

推荐做法：

- 扫描时优先使用 Service UUID 过滤。
- 连接后校验三个 UUID 都存在。
- 先订阅 Response Notify，确认订阅成功后再写 Command。
- 未订阅 Notify 时，可以 Read Response 读取设备保存的最后一条响应，但无法可靠接收连续两次异步响应。

## 3. 广播与设备匹配

| 字段 | 内容 |
|---|---|
| Local Name | `<设备型号>-<设备序列号>`，例如 `TV-3852TLW-224a958600034021` |
| Service UUID | `7A1E0001-7D8B-4F31-9E45-545650524F56` |
| Advertising Type | Peripheral |

二维码建议包含：

```json
{
  "v": 1,
  "model": "TV-3852TLW",
  "sn": "224a958600034021",
  "service": "7A1E0001-7D8B-4F31-9E45-545650524F56",
  "setupSecret": "64个十六进制字符"
}
```

App 匹配设备的建议顺序：

1. 过滤 Provisioning Service UUID。
2. 检查 Local Name 是否等于 `<model>-<sn>`。
3. 用户确认后再连接。

不要依赖 BLE MAC 地址进行设备匹配；iOS 不向 App 提供设备的真实蓝牙 MAC 地址。

## 4. 通用帧格式

### 4.1 帧布局

| 偏移 | 字段 | 长度 | 请求值 | 响应值 | 说明 |
|---:|---|---:|---|---|---|
| 0 | Magic | 2 | `54 56` | `54 56` | ASCII `TV` |
| 2 | Version | 1 | `01` | `01` | 当前协议版本为 1 |
| 3 | Flags | 1 | `00` | `01` | bit0=1 表示响应帧 |
| 4 | Command | 2 | 指令码 | 原请求指令码 | 大端序 |
| 6 | Sequence | 2 | App 生成 | 原请求序号 | 大端序 |
| 8 | Payload Length | 2 | N | M | 不包含 CRC，最大 1024 字节 |
| 10 | Payload | N | 指令数据 | Status + Data | 可以为空 |
| 10+N | CRC16 | 2 | CRC | CRC | 大端序 |

```text
总帧长度 = 10字节固定头 + Payload Length + 2字节CRC
         = 12 + Payload Length
```

### 4.2 请求和响应 Payload

请求 Payload 由具体指令定义。响应 Payload 始终是：

```text
status_BE[2] || responseData[N]
```

因此，响应的业务 Data 从完整帧偏移 12 开始，而不是偏移 10。

### 4.3 Sequence

- App 为每个新请求分配一个 `uint16` sequence。
- 建议逐次递增；溢出后可以从 0 重新循环。
- 设备响应会原样带回请求的 command 和 sequence。
- 同一异步请求的“受理响应”和“最终响应”使用相同 command、sequence。
- App 应使用 `(command, sequence)` 作为请求关联键。
- `sequence` 不提供安全防重放能力。

### 4.4 完整数据报文格式

本节描述最终写入 Command Characteristic、或从 Response Characteristic 收到的完整二进制数据。表中的偏移均从完整帧第一个字节开始计算。

> [!NOTE]
> 文档中的 `54 56 01 ...` 是为了阅读而显示的十六进制。App 必须将其转换成 `byte[]` 后发送，不能发送字符 `'5'`、`'4'`，也不能发送 `0x5456...` 字符串。

#### 4.4.1 通用请求帧

```text
┌────────────── 10字节固定头 ──────────────┐
│ Magic │ Ver │ Flags │ Cmd │ Seq │ Length │ Request Payload │ CRC16 │
│  2B   │ 1B  │  1B   │ 2B  │ 2B  │   2B   │       N B       │  2B   │
└───────────────────────────────────────────┘
```

| 绝对偏移 | 字段 | 长度 | 请求取值 |
|---:|---|---:|---|
| 0 | Magic | 2 | 固定 `54 56` |
| 2 | Version | 1 | 固定 `01` |
| 3 | Flags | 1 | 固定 `00` |
| 4 | Command | 2 | 指令码，大端序 |
| 6 | Sequence | 2 | App 请求序号，大端序 |
| 8 | Payload Length | 2 | N，大端序 |
| 10 | Request Payload | N | 由具体指令定义 |
| 10+N | CRC16 | 2 | 对偏移 `0`～`9+N` 计算，大端序 |

```text
请求总长度 = 12 + N
```

#### 4.4.2 通用响应帧

```text
┌────────────── 10字节固定头 ──────────────┐
│ Magic │ Ver │ Flags │ Cmd │ Seq │ Length │ Status │ Response Data │ CRC16 │
│  2B   │ 1B  │  1B   │ 2B  │ 2B  │   2B   │   2B   │      M B      │  2B   │
└───────────────────────────────────────────┘
```

| 绝对偏移 | 字段 | 长度 | 响应取值 |
|---:|---|---:|---|
| 0 | Magic | 2 | 固定 `54 56` |
| 2 | Version | 1 | 固定 `01` |
| 3 | Flags | 1 | 固定 `01` |
| 4 | Command | 2 | 与请求一致 |
| 6 | Sequence | 2 | 与请求一致 |
| 8 | Payload Length | 2 | `2 + M`，包含 Status |
| 10 | Status | 2 | 协议状态码，大端序 |
| 12 | Response Data | M | 由具体指令定义，可以为空 |
| 12+M | CRC16 | 2 | 对偏移 `0`～`11+M` 计算，大端序 |

```text
响应Payload长度 = 2 + M
响应总长度      = 14 + M
```

当 Data 为空时，响应固定为 14 字节：

```text
54 56 01 01 CC CC SS SS 00 02 TT TT RR RR
            └Cmd┘ └Seq┘       └状态┘ └CRC┘
```

其中 `CC CC`、`SS SS`、`TT TT`、`RR RR` 分别代表 command、sequence、status 和 CRC，不是固定值。

#### 4.4.3 `0x0010` 开始认证报文

请求 Payload：

```text
appNonce[16]
```

| 绝对偏移 | 字段 | 长度 |
|---:|---|---:|
| 0～9 | 通用请求头 | 10 |
| 10～25 | appNonce | 16 |
| 26～27 | CRC16 | 2 |

```text
Payload Length = 16（0x0010）
完整请求长度   = 28字节
```

成功响应 Payload：

```text
status_BE[2] || sessionId_BE[4] || deviceNonce[16]
```

| 绝对偏移 | 字段 | 长度 |
|---:|---|---:|
| 0～9 | 通用响应头 | 10 |
| 10～11 | Status | 2 |
| 12～15 | sessionId | 4 |
| 16～31 | deviceNonce | 16 |
| 32～33 | CRC16 | 2 |

```text
Payload Length = 22（0x0016）
完整响应长度   = 34字节
```

只有 `Status == STATUS_OK` 时，偏移 12 之后才可以按照 sessionId、deviceNonce 解析。

#### 4.4.4 `0x0011` 提交认证证明报文

请求 Payload：

```text
sessionId_BE[4] || proof[32]
```

| 绝对偏移 | 字段 | 长度 |
|---:|---|---:|
| 0～9 | 通用请求头 | 10 |
| 10～13 | sessionId | 4 |
| 14～45 | HMAC-SHA256 Proof | 32 |
| 46～47 | CRC16 | 2 |

```text
Payload Length = 36（0x0024）
完整请求长度   = 48字节
```

成功响应 Payload：

```text
status_BE[2] || ASCII("authenticated")[13]
```

```text
Payload Length = 15（0x000F）
完整响应长度   = 27字节
```

#### 4.4.5 安全业务请求报文

适用于 `0x0101`～`0x0202`。设原始业务明文长度为 N，加密后 ciphertext 长度同样为 N。

```text
┌────────────── 通用请求头 ──────────────┐
│ Magic │ Ver │ Flags │ Cmd │ Seq │ Length │
└─────────────────────────────────────────┘
│ sessionId │ counter │   nonce   │ ciphertext │   tag   │ CRC16 │
│     4B    │   4B    │    12B    │     N B    │   16B   │  2B   │
```

| 绝对偏移 | Payload内偏移 | 字段 | 长度 | 是否加密 |
|---:|---:|---|---:|---|
| 0～9 | — | 通用请求头 | 10 | 否 |
| 10～13 | 0～3 | sessionId | 4 | 否，参与 AAD |
| 14～17 | 4～7 | counter | 4 | 否，参与 AAD |
| 18～29 | 8～19 | nonce | 12 | 否 |
| 从 30 开始 | 从 20 开始 | ciphertext | N | 是，原始业务 Data 的密文；N=0 时该字段不存在 |
| 30+N～45+N | 20+N～35+N | GCM Tag | 16 | 否，用于认证 |
| 46+N～47+N | — | CRC16 | 2 | 否 |

```text
安全Payload长度 = 4 + 4 + 12 + N + 16 = 36 + N
完整请求长度    = 10 + (36 + N) + 2 = 48 + N
```

限制：

- 协议 Payload 最大 1024 字节，所以安全业务明文最大为 `1024 - 36 = 988` 字节。
- AAD 不单独放入报文，它由 App 和设备根据头部及安全字段各自构造。
- CRC 在完成 AES-GCM 加密并拼好安全 Payload 后，最后计算。
- AES-GCM Tag 校验通过后，设备才会把 ciphertext 解密结果交给业务模块。

空业务 Data（例如 Wi-Fi 扫描、连接、状态查询）的完整布局：

```text
偏移 00: 54 56                         Magic
偏移 02: 01                            Version
偏移 03: 00                            Request Flags
偏移 04: CC CC                         Command
偏移 06: SS SS                         Sequence
偏移 08: 00 24                         Payload Length = 36
偏移 10: II II II II                   sessionId
偏移 14: NN NN NN NN                   counter
偏移 18: <12字节nonce>
偏移 30: <16字节GCM tag>               无ciphertext
偏移 46: RR RR                         CRC16

完整请求长度 = 48字节
```

#### 4.4.6 安全业务响应报文

当前协议的响应不使用 AES-GCM，仍采用 4.4.2 的通用明文响应格式：

```text
通用响应头 || status_BE[2] || responseData[M] || CRC16
```

注意：

- “响应不加密”不代表请求可以不加密。
- 响应 Data 不包含 setupSecret、sessionKey、Wi-Fi 密码或平台密码。
- App 应先检查 Status，再按照具体指令解析 Data。
- 响应 Data 最大长度为 `1024 - 2 = 1022` 字节。

#### 4.4.7 异步指令的两种响应格式

异步指令 `0x0102`、`0x0105`、`0x0201` 首先返回受理响应：

```text
Magic || Version || 01 || Command || Sequence || 00 02
|| 00 00
|| CRC16
```

受理响应固定为 14 字节，其中 `Status=0x0000`、Data 为空。

任务完成后，设备再次使用相同 Command、Sequence 返回最终响应：

```text
Magic || Version || 01 || Command || Sequence || (2+M)_BE
|| finalStatus_BE[2]
|| finalData[M]
|| CRC16
```

App 不能仅凭相同的 Status 判断两个响应，必须同时根据 Data 是否为空和当前请求状态机判断“受理”与“最终结果”。

## 5. CRC16

| 参数 | 值 |
|---|---|
| 算法 | CRC-16/CCITT-FALSE |
| Polynomial | `0x1021` |
| Init | `0xFFFF` |
| RefIn / RefOut | false / false |
| XorOut | `0x0000` |
| 覆盖范围 | 从 Magic 到 Payload 最后一个字节 |
| CRC 字节序 | 大端序 |

参考实现：

```c
uint16_t ble_crc16(const uint8_t *data, size_t size)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < size; ++i)
    {
        crc ^= (uint16_t)data[i] << 8;
        for (int bit = 0; bit < 8; ++bit)
            crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021)
                                 : (uint16_t)(crc << 1);
    }
    return crc;
}
```

## 6. 响应解析

一次 Notify 只是完整响应帧的一段，不保证从帧头开始，也不保证包含完整帧。App 必须为每条 BLE 连接维护一个接收缓存，将每次 Notify 的原始字节按到达顺序追加；只有缓存达到完整帧长度后才能按以下步骤解析。

```text
onNotify(fragment):
    receiveBuffer.append(fragment)

    while receiveBuffer.length >= 10:
        if receiveBuffer[0..1] != 54 56:
            删除首字节并继续寻找帧头
            continue

        payloadLength = readU16BE(receiveBuffer, 8)
        if payloadLength > 1024:
            删除首字节并继续寻找帧头
            continue

        expectedLength = 12 + payloadLength
        if receiveBuffer.length < expectedLength:
            等待下一次 Notify
            return

        frame = receiveBuffer[0 .. expectedLength-1]
        从缓存删除 expectedLength 字节
        校验 CRC 并解析 frame
```

完整帧组装完成后按以下顺序处理：

1. 长度至少为 12 字节。
2. `frame[0..1] == 54 56`。
3. `frame[2] == 01`。
4. `(frame[3] & 0x01) == 1`。
5. 大端读取 command、sequence、payloadLength。
6. 检查 `frame.length == 12 + payloadLength`。
7. 校验末尾 CRC16。
8. 响应 `payloadLength >= 2`。
9. 大端读取 Payload 前两个字节为 status。
10. 剩余字节为 responseData。
11. 使用 command、sequence 查找等待中的请求。

```text
parseResponse(frame):
    require frame.length >= 12
    require frame[0] == 0x54 && frame[1] == 0x56
    require frame[2] == 0x01
    require (frame[3] & 0x01) == 1

    command       = readU16BE(frame, 4)
    sequence      = readU16BE(frame, 6)
    payloadLength = readU16BE(frame, 8)

    require frame.length == 12 + payloadLength
    require readU16BE(frame, frame.length - 2) ==
            crc16(frame[0 .. frame.length - 3])
    require payloadLength >= 2

    status = readU16BE(frame, 10)
    data   = frame[12 .. frame.length - 3]
    return { command, sequence, status, data }
```

## 7. 状态码

| 值 | 名称 | App 处理建议 |
|---:|---|---|
| `0x0000` | `STATUS_OK` | 成功；异步指令可能仅表示已受理 |
| `0x0001` | `STATUS_BAD_FRAME` | 检查帧头、长度、分片顺序 |
| `0x0002` | `STATUS_BAD_CRC` | 重新计算 CRC 后重发新请求 |
| `0x0003` | `STATUS_UNSUPPORTED_VERSION` | 使用协议版本 1 |
| `0x0004` | `STATUS_UNKNOWN_COMMAND` | 检查指令码或固件能力 |
| `0x0005` | `STATUS_INVALID_PAYLOAD` | 检查 JSON、字段或 Payload 长度 |
| `0x0006` | `STATUS_BUSY` | 等待当前操作完成，或先完成前置步骤 |
| `0x0007` | `STATUS_NOT_IMPLEMENTED` | 当前固件未实现 |
| `0x0008` | `STATUS_INTERNAL_ERROR` | 设备执行或业务投递失败，查询状态后决定是否重试 |
| `0x0009` | `STATUS_UNAUTHORIZED` | 未认证、会话过期或 sessionId 错误；重新认证 |
| `0x000A` | `STATUS_AUTH_FAILED` | HMAC 证明错误或 GCM Tag 校验失败 |
| `0x000B` | `STATUS_AUTH_LOCKED` | 认证失败次数过多，等待 60 秒 |
| `0x000C` | `STATUS_REPLAY_DETECTED` | counter 重复或倒退；不要复用旧密文 |

## 8. 二维码认证和加密

### 8.1 setupSecret

每台设备具有独立的 32 字节 `setupSecret`，文件中保存为 64 个十六进制字符：

```text
/opt/course/config/ble_setup_secret
```

要求：

- 量产时必须保证每台设备的 setupSecret 不同。
- setupSecret 只能通过受控生产流程写入二维码，不能放入广播。
- App 不得记录 setupSecret、sessionKey、认证 Proof、Wi-Fi 密码或平台密码。
- App 日志只记录 command、sequence、status、Payload 长度等非敏感信息。

### 8.2 `0x0010` 开始认证

App 使用密码学安全随机数生成器创建 16 字节 `appNonce`：

```text
requestData = appNonce[16]
```

设备响应 Data：

```text
sessionId_BE[4] || deviceNonce[16]
```

完整响应 Payload 仍包含前置 Status：

```text
status_BE[2] || sessionId_BE[4] || deviceNonce[16]
```

### 8.3 `0x0011` 提交认证证明

App 计算：

```text
proof = HMAC-SHA256(
    key  = setupSecret[32],
    data = ASCII("TV-BLE-AUTH-V1")
           || sessionId_BE[4]
           || appNonce[16]
           || deviceNonce[16]
)
```

请求 Data：

```text
sessionId_BE[4] || proof[32]
```

成功响应：

```text
status = 0x0000
data   = ASCII("authenticated")
```

认证挑战和认证会话有效期均为 5 分钟；每次成功解密业务请求后，会话续期 5 分钟。连续认证失败 5 次后锁定 60 秒。

### 8.4 会话密钥派生

认证过程中 App 和设备使用相同算法派生 32 字节 sessionKey：

```text
salt = appNonce[16] || deviceNonce[16]

PRK = HMAC-SHA256(
    key  = salt,
    data = setupSecret
)

sessionKey = HMAC-SHA256(
    key  = PRK,
    data = ASCII("TV-BLE-PROVISION-V1") || 0x01
)
```

### 8.5 安全业务 Payload

认证成功后，`0x0101`～`0x0202` 请求的协议 Payload 必须是：

```text
sessionId_BE[4]
|| counter_BE[4]
|| nonce[12]
|| ciphertext[N]
|| tag[16]
```

| 字段 | 长度 | 规则 |
|---|---:|---|
| sessionId | 4 | `0x0010` 返回值，大端序 |
| counter | 4 | 从 1 开始严格递增，大端序 |
| nonce | 12 | 每条请求唯一；同一 sessionKey 下绝不能重复 |
| ciphertext | N | AES-256-GCM 加密后的原始业务 Data |
| tag | 16 | GCM Authentication Tag |

AES-GCM 的 AAD：

```text
version[1]
|| command_BE[2]
|| sequence_BE[2]
|| sessionId_BE[4]
|| counter_BE[4]
```

加密伪代码：

```text
encryptBusinessPayload(command, sequence, plainData):
    counter = nextCounter()                  // 首次为1，严格递增
    nonce   = secureRandom(12)              // 不可重复
    aad     = 0x01
              || u16be(command)
              || u16be(sequence)
              || u32be(sessionId)
              || u32be(counter)

    ciphertext, tag = AES_256_GCM_ENCRYPT(
        key=sessionKey,
        nonce=nonce,
        aad=aad,
        plaintext=plainData,
        tagLength=16)

    return u32be(sessionId)
           || u32be(counter)
           || nonce
           || ciphertext
           || tag
```

原始业务 Data 为空时，`ciphertext` 长度为 0，但 sessionId、counter、nonce 和 tag 仍必须存在。此时安全 Payload 长度为 36 字节。

> [!WARNING]
> 重试安全指令时必须创建新请求：分配新 sequence、递增 counter、生成新 nonce 并重新加密。不要原样重发旧密文，否则会返回 `STATUS_REPLAY_DETECTED`。

## 9. 指令总表

| 指令码 | 名称 | 是否加密 | 原始业务 Data | 响应方式 |
|---:|---|---|---|---|
| `0x0001` | 获取协议信息 | 否 | 空 | 同步一次 |
| `0x0002` | 心跳 | 否 | 任意 0～1024 字节 | 同步一次 |
| `0x0010` | 开始认证 | 否 | appNonce 16 字节 | 同步一次 |
| `0x0011` | 提交认证证明 | 否 | sessionId + HMAC | 同步一次 |
| `0x0012` | 查询认证状态 | 否 | 空 | 同步一次 |
| `0x0101` | 保存 Wi-Fi 凭据 | 是 | Wi-Fi JSON | 同步一次 |
| `0x0102` | 连接 Wi-Fi | 是 | 空 | 受理响应 + 最终响应 |
| `0x0103` | 查询 Wi-Fi 状态 | 是 | 空 | 同步一次 |
| `0x0104` | 取消 Wi-Fi 配网 | 是 | 空 | 当前固件未注册，返回未知指令 |
| `0x0105` | 扫描附近 Wi-Fi | 是 | 空 | 受理响应 + 最终响应 |
| `0x0201` | 配置并连接平台 | 是 | 平台 JSON | 受理响应 + 最终响应 |
| `0x0202` | 查询平台状态 | 是 | 空 | 同步一次 |
| `0x0300`～`0x6FFF` | 公共功能预留 | 待定义 | 待定义 | 待定义 |
| `0x7000`～`0x7FFF` | 厂商扩展预留 | 待定义 | 待定义 | 待定义 |

## 10. 指令详细定义

### 10.1 `0x0001` 获取协议信息

请求 Data：空。

当前返回 Data：

```text
TV-BLE/1;frame=crc16;max=1024;auth=hmac-sha256;cipher=aes-256-gcm
```

App 应检查主版本为 `TV-BLE/1`，并支持 `auth=hmac-sha256` 和 `cipher=aes-256-gcm`。

### 10.2 `0x0002` 心跳

请求 Data：任意二进制数据。设备原样返回，可用于测试帧编解码和 Notify 通道。

请求 `ping`，sequence=`0x0002`：

```text
54 56 01 00 00 02 00 02 00 04 70 69 6E 67 25 94
```

响应：

```text
54 56 01 01 00 02 00 02 00 06 00 00 70 69 6E 67 7D 0A
```

### 10.3 `0x0012` 查询认证状态

请求 Data：空。响应 Data 为 `authenticated` 或 `unauthenticated`。

### 10.4 `0x0101` 保存 Wi-Fi 凭据

原始业务 Data 是 UTF-8 JSON，推荐使用紧凑 JSON：

```json
{"ssid":"ExampleWiFi","password":"ExamplePassword","pairwise":"CCMP","mode":"WPA2","wpa3Type":""}
```

| 字段 | 类型 | 必填 | 说明 |
|---|---|---|---|
| `ssid` | string | 是 | Wi-Fi 名称 |
| `password` | string | 加密网络必填 | Wi-Fi 密码；开放网络可以为空 |
| `pairwise` | string | 建议 | `CCMP` 或 `TKIP` |
| `mode` | string | 是 | `OPEN`、`WEP`、`WPA`、`WPA2`、`WPA_PERSONAL`、`WPA3` |
| `wpa3Type` | string | 企业网络按需 | 空、`EAP_PEAP`、`EAP_TLS`、`EAP_TTLS` |

当前 BLE 层只检查 Data 是非空的 `{...}` JSON 文本，然后暂存到本次配网流程；真正连接由 `0x0102` 触发。字段合法性和网络支持情况最终由 Wi-Fi 业务模块决定。

#### 10.4.1 中文及其他非 ASCII SSID

本协议支持中文 SSID。App、设备协议层、JSON 和 Wi-Fi 模块之间统一使用 UTF-8；不使用 GBK、GB2312，也不能把十六进制显示文本当作 SSID 发送。

推荐 App 直接发送 UTF-8 JSON：

```json
{"ssid":"智慧教室","password":"ExamplePassword","pairwise":"CCMP","mode":"WPA2","wpa3Type":""}
```

也兼容标准 JSON Unicode 转义：

```json
{"ssid":"\u667a\u6167\u6559\u5ba4","password":"ExamplePassword","pairwise":"CCMP","mode":"WPA2","wpa3Type":""}
```

设备 JSON 解析器会把 `\uXXXX` 转换为 UTF-8。为便于联调和避免重复转义，建议 App 优先直接使用 UTF-8 JSON。

App 构造请求时必须遵循以下顺序：

```text
JSON对象
  ↓ UTF-8序列化（不带BOM）
UTF-8 JSON byte[]
  ↓ 作为原始业务明文进行AES-256-GCM加密
安全Payload
  ↓ 构造Header、Length、CRC16
完整BLE协议帧
  ↓ 按最大写入长度分片
写入Command Characteristic
```

BLE 分片是按字节进行的，因此一个中文字符的 UTF-8 字节可能被拆到两个 BLE 分片中。这不会造成乱码：设备先恢复完整协议帧、完成解密，再把完整 UTF-8 JSON交给解析器。App 不得把每个分片单独转换为字符串。

##### SSID 长度限制

IEEE 802.11 SSID 上限是 32 字节，不是32个 Unicode 字符。App 必须对 `ssid` 的 UTF-8 编码结果计算字节数：

```text
ssidBytes = UTF8(ssid)
要求：1 <= ssidBytes.length <= 32
```

常用中文汉字通常占3个 UTF-8 字节。例如：

```text
“智慧教室” = E6 99 BA E6 85 A7 E6 95 99 E5 AE A4
             = 12字节

10个常用汉字通常为30字节，可以作为SSID
11个常用汉字通常为33字节，超过SSID上限
```

不能使用 Java/Kotlin 的 `String.length`、Swift 的 `String.count` 或 JavaScript 的字符串长度直接判断；这些通常是字符数或 UTF-16 单元数，不是 UTF-8 字节数。

App 还应拒绝以下输入：

- 空 SSID；
- UTF-8 编码后超过32字节；
- 包含 U+0000/NUL；
- 非法 UTF-8、未配对代理项或错误的 JSON 转义；
- 把 `%E6%99%BA...`、`E699BA...` 等 URL/HEX 显示形式直接放入 `ssid` 字段。

设备 Wi-Fi 模块检测到非 ASCII SSID 后，会把收到的 UTF-8原始字节转换为 `wpa_supplicant` 所需的十六进制 SSID。例如：

```text
App JSON中的SSID：智慧教室
wpa_supplicant配置：ssid=E699BAE685A7E69599E5AEA4
```

该十六进制转换由设备完成，App 不需要也不应该提前转换。

> [!WARNING]
> 当前固件会正确透传并配置合法 UTF-8 SSID，但 BLE入口尚未对 UTF-8 合法性和32字节上限做强制校验。App 必须先完成上述校验；异常输入最终可能表现为 `wifi_connect_failed`。

### 10.5 `0x0102` 连接 Wi-Fi

原始业务 Data：空。

```text
受理：status=0x0000, data=空
成功：status=0x0000, data=state=1;detail=wifi_connected
失败：status=0x0008, data=state=0;detail=wifi_connect_failed
```

最终业务状态建议再通过 `0x0103` 确认。

### 10.6 `0x0103` 查询 Wi-Fi 状态

原始业务 Data：空。返回 Data：

```text
state=<状态值>;detail=<描述>
```

| state | 含义 | 常见 detail |
|---:|---|---|
| 0 | 尚未配置 | 空 |
| 1 | 凭据已保存 | `credentials_ready` |
| 2 | 正在连接 | `connecting` |
| 3 | 已连接 | `wifi_connected` |
| 4 | 连接失败 | `wifi_connect_failed`、`dispatch_failed` |

示例响应：

```text
54 56 01 01 01 03 00 03 00 1F 00 00
73 74 61 74 65 3D 33 3B 64 65 74 61 69 6C 3D
77 69 66 69 5F 63 6F 6E 6E 65 63 74 65 64
C4 09
```

解析为 `status=0x0000`、`data=state=3;detail=wifi_connected`。

### 10.7 `0x0105` 扫描附近 Wi-Fi

原始业务 Data：空。App 仍需为“空明文”生成完整的 AES-GCM 安全 Payload。

设备先返回 `STATUS_OK + 空 Data`，扫描完成后再返回 UTF-8 JSON：

```json
{
  "count": 3,
  "truncated": false,
  "list": [
    {"ssid":"Office","security":"WPA2"},
    {"ssid":"Guest","security":"WPA2"},
    {"ssid":"TPGuest_910D","security":"Open"}
  ]
}
```

中文扫描结果示例：

```json
{
  "count": 2,
  "truncated": false,
  "list": [
    {"ssid":"智慧教室","security":"WPA2"},
    {"ssid":"访客网络","security":"Open"}
  ]
}
```

| 字段 | 类型 | 说明 |
|---|---|---|
| `count` | integer | `list` 实际元素数量 |
| `truncated` | boolean | `true` 表示结果超过返回上限 |
| `list` | array | Wi-Fi 列表，最多 6 项 |
| `list[].ssid` | string | UTF-8 Wi-Fi 名称；隐藏 SSID 不返回 |
| `list[].security` | string | `Open`、`WEP`、`WPA`、`WPA2`、`WPA3` 或 `Unknown` |

当前固件不会返回 `rssi`、`band` 或 `current`。App 不得依赖这些字段。

设备会对同名且加密方式相同的热点去重，最多返回 6 项。`truncated=true` 时 App 可以允许用户手动输入未显示的 SSID。扫描失败时最终响应为 `STATUS_INTERNAL_ERROR`。

扫描结果中的 `ssid` 已经是 UTF-8 JSON 字符串。App 应先完成 Notify 分片组帧和 CRC 校验，再对完整 `responseData` 做一次 UTF-8 JSON解析；不能逐个 Notify 分片解码中文。显示给用户时应保留设备返回的 UTF-8 字节含义，不要擅自执行 GBK 转码或把名称转换为 HEX。

#### 10.7.1 从认证到 Wi-Fi 扫描的完整测试向量

本节给出一套完整、可复算的单元测试数据。所有数值均为公开测试值，不是任何真实设备的生产密钥。

> [!CAUTION]
> 这组固定值用于 App 单元测试和理解协议。真实联调时，`deviceNonce` 和 `sessionId` 由设备随机返回，App 必须使用设备实际返回值重新计算 Proof、sessionKey 和扫描密文，不能直接复制本节报文发送给真实设备。

固定输入：

```text
setupSecret =
000102030405060708090A0B0C0D0E0F
101112131415161718191A1B1C1D1E1F

appNonce = 101112131415161718191A1B1C1D1E1F

示例设备返回：
sessionId  = 12345678
deviceNonce = A0A1A2A3A4A5A6A7A8A9AAABACADAEAF
```

##### 第一步：发送 `0x0010` 开始认证

请求参数：

```text
command  = 0x0010
sequence = 0x0001
payload  = appNonce[16]
```

完整请求帧：

```text
54 56 01 00 00 10 00 01 00 10
10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F
DF 6A
```

示例设备响应：

```text
54 56 01 01 00 10 00 01 00 16 00 00
12 34 56 78
A0 A1 A2 A3 A4 A5 A6 A7 A8 A9 AA AB AC AD AE AF
13 E1
```

响应解析：

```text
status      = 0x0000
sessionId   = 0x12345678
deviceNonce = A0A1A2A3A4A5A6A7A8A9AAABACADAEAF
```

##### 第二步：计算并发送 `0x0011` 认证证明

按照 8.3 节的 HMAC 输入顺序计算：

```text
proof = D929FA836B61F5CE29B2C76DB4596E54
        94E6154C9F73E9313172077D2CEECA31
```

请求 Payload：

```text
12345678
D929FA836B61F5CE29B2C76DB4596E54
94E6154C9F73E9313172077D2CEECA31
```

完整请求帧：

```text
54 56 01 00 00 11 00 02 00 24
12 34 56 78
D9 29 FA 83 6B 61 F5 CE 29 B2 C7 6D B4 59 6E 54
94 E6 15 4C 9F 73 E9 31 31 72 07 7D 2C EE CA 31
AF 9A
```

认证成功响应：

```text
54 56 01 01 00 11 00 02 00 0F 00 00
61 75 74 68 65 6E 74 69 63 61 74 65 64
51 B9
```

解析结果：

```text
status = 0x0000
data   = authenticated
```

##### 第三步：派生 sessionKey

按照 8.4 节的算法计算，得到：

```text
sessionKey = 23D210D8E2D26AE2F79A2B807964BB68
             2345B8EE532733A880486A6AC4AB042D
```

App 应在内存中保存 sessionKey，不得打印到正式日志。这里列出它仅用于测试实现是否正确。

##### 第四步：构造加密的 `0x0105` Wi-Fi 扫描请求

扫描指令的原始业务 Data 为空。本测试使用：

```text
command   = 0x0105
sequence  = 0x0003
counter   = 0x00000001
nonce     = 000102030405060708090A0B
plaintext = 空

AAD = 01 01 05 00 03 12 34 56 78 00 00 00 01
```

AES-256-GCM 输出：

```text
ciphertext = 空
tag        = 17356B8595628E434FD3E3D2730EAA55
```

安全 Payload：

```text
12 34 56 78                         // sessionId
00 00 00 01                         // counter
00 01 02 03 04 05 06 07 08 09 0A 0B // nonce
17 35 6B 85 95 62 8E 43 4F D3 E3 D2 73 0E AA 55 // tag
```

完整扫描请求帧：

```text
54 56 01 00 01 05 00 03 00 24
12 34 56 78 00 00 00 01
00 01 02 03 04 05 06 07 08 09 0A 0B
17 35 6B 85 95 62 8E 43 4F D3 E3 D2 73 0E AA 55
9E D6
```

##### 第五步：接收扫描受理响应

设备首先返回：

```text
54 56 01 01 01 05 00 03 00 02 00 00 B9 81
```

解析结果：

```text
command  = 0x0105
sequence = 0x0003
status   = 0x0000
data     = 空
含义     = 扫描任务已受理，尚未完成
```

App 此时必须继续等待相同 command、sequence 的第二条响应。

##### 第六步：接收扫描最终响应

下面使用此前设备实际扫描得到的 JSON 数据，并把 sequence 调整为本测试流程的 `0x0003` 后重新计算 CRC：

```text
5456010101050003015E00007B0A0922636F756E74223A09362C0A0922747275
6E6361746564223A09747275652C0A09226C697374223A095B7B0A0909092273
736964223A09224F6666696365222C0A090909227365637572697479223A0922
57504132220A09097D2C207B0A0909092273736964223A09224775657374222C
0A090909227365637572697479223A092257504132220A09097D2C207B0A0909
092273736964223A0922434D572D4150222C0A09090922736563757269747922
3A092257504132220A09097D2C207B0A0909092273736964223A092254504775
6573745F39313044222C0A090909227365637572697479223A09224F70656E22
0A09097D2C207B0A0909092273736964223A0922736A325F3567222C0A090909
227365637572697479223A092257504132220A09097D2C207B0A090909227373
6964223A0922483343222C0A090909227365637572697479223A09224F70656E
220A09097D5D0A7DE24A
```

解析结果：

```text
command       = 0x0105
sequence      = 0x0003
payloadLength = 0x015E（350字节）
status        = 0x0000
CRC16         = 0xE24A
```

Data 按 UTF-8 解码并解析 JSON：

```json
{
  "count": 6,
  "truncated": true,
  "list": [
    {"ssid":"Office","security":"WPA2"},
    {"ssid":"Guest","security":"WPA2"},
    {"ssid":"CMW-AP","security":"WPA2"},
    {"ssid":"TPGuest_910D","security":"Open"},
    {"ssid":"sj2_5g","security":"WPA2"},
    {"ssid":"H3C","security":"Open"}
  ]
}
```

至此，一次完整的“认证并扫描 Wi-Fi”流程结束。下一条安全指令必须使用 `counter=2`、新的 nonce 和新的 sequence。

#### 10.7.2 设备实际验证的 Wi-Fi 扫描响应

下面是设备实际扫描成功后返回的第二条（最终）响应。为便于阅读进行了换行；App 实际收到的是一段连续二进制数据：

```text
54 56 01 01 01 05 00 04 01 5E 00 00
7B 0A 09 22 63 6F 75 6E 74 22 3A 09 36 2C 0A 09
22 74 72 75 6E 63 61 74 65 64 22 3A 09 74 72 75
65 2C 0A 09 22 6C 69 73 74 22 3A 09 5B 7B 0A 09
09 09 22 73 73 69 64 22 3A 09 22 4F 66 66 69 63
65 22 2C 0A 09 09 09 22 73 65 63 75 72 69 74 79
22 3A 09 22 57 50 41 32 22 0A 09 09 7D 2C 20 7B
0A 09 09 09 22 73 73 69 64 22 3A 09 22 47 75 65
73 74 22 2C 0A 09 09 09 22 73 65 63 75 72 69 74
79 22 3A 09 22 57 50 41 32 22 0A 09 09 7D 2C 20
7B 0A 09 09 09 22 73 73 69 64 22 3A 09 22 43 4D
57 2D 41 50 22 2C 0A 09 09 09 22 73 65 63 75 72
69 74 79 22 3A 09 22 57 50 41 32 22 0A 09 09 7D
2C 20 7B 0A 09 09 09 22 73 73 69 64 22 3A 09 22
54 50 47 75 65 73 74 5F 39 31 30 44 22 2C 0A 09
09 09 22 73 65 63 75 72 69 74 79 22 3A 09 22 4F
70 65 6E 22 0A 09 09 7D 2C 20 7B 0A 09 09 09 22
73 73 69 64 22 3A 09 22 73 6A 32 5F 35 67 22 2C
0A 09 09 09 22 73 65 63 75 72 69 74 79 22 3A 09
22 57 50 41 32 22 0A 09 09 7D 2C 20 7B 0A 09 09
09 22 73 73 69 64 22 3A 09 22 48 33 43 22 2C 0A
09 09 09 22 73 65 63 75 72 69 74 79 22 3A 09 22
4F 70 65 6E 22 0A 09 09 7D 5D 0A 7D F1 C8
```

帧头解析：

| 字节 | 解析值 | 含义 |
|---|---|---|
| `54 56` | `TV` | Magic |
| `01` | 1 | 协议版本 |
| `01` | Response | 响应标志 |
| `01 05` | `0x0105` | Wi-Fi 扫描指令 |
| `00 04` | 4 | 与扫描请求相同的 sequence |
| `01 5E` | 350 | 响应 Payload 长度，包含 2 字节 Status |
| `00 00` | `STATUS_OK` | 扫描成功 |
| 后续 348 字节 | UTF-8 JSON | 扫描结果 Data |
| `F1 C8` | CRC16 | 完整帧校验值 |

将 Status 后面的 Data 按 UTF-8 解码并解析 JSON，得到：

```json
{
  "count": 6,
  "truncated": true,
  "list": [
    {"ssid":"Office","security":"WPA2"},
    {"ssid":"Guest","security":"WPA2"},
    {"ssid":"CMW-AP","security":"WPA2"},
    {"ssid":"TPGuest_910D","security":"Open"},
    {"ssid":"sj2_5g","security":"WPA2"},
    {"ssid":"H3C","security":"Open"}
  ]
}
```

这个例子中 `count=6` 与 `list` 数量一致；`truncated=true` 表示设备还扫描到了其他热点，但受到最多返回 6 项的限制而被截断。App 应正常显示这 6 项，并允许用户手动输入列表中没有的 SSID。

### 10.8 `0x0201` 配置并连接平台

只有 Wi-Fi 状态为 `state=3` 时允许执行，否则返回 `STATUS_BUSY`。

原始业务 Data 示例：

```json
{"server_ip":"192.0.2.10","server_port":4910,"RtmpPort":4920,"MqttPort":1884,"user":"demo","password":"ExamplePassword","enable":true,"Custom":true}
```

| 字段 | 类型 | 必填 | 说明 |
|---|---|---|---|
| `server_ip` | string | 是 | 平台 IP 或业务层支持的服务器地址 |
| `server_port` | integer/string | 是 | HTTP/API 端口，1～65535 |
| `RtmpPort` | integer/string | 是 | RTMP 端口，1～65535 |
| `MqttPort` | integer/string | 是 | MQTT 端口，1～65535 |
| `user` | string | 是 | 平台用户名 |
| `password` | string | 是 | 平台密码 |
| `enable` | boolean | 是 | 是否启用平台 |
| `Custom` | boolean | 是 | 是否使用自定义平台参数 |

设备先返回受理响应，平台业务完成后再返回最终结果。平台连接成功后，设备约等待 3 秒，随后注销 GATT、停止广播并关闭蓝牙。App 此时收到 BLE 断开属于正常配网完成流程。

### 10.9 `0x0202` 查询平台状态

原始业务 Data：空。返回 Data：

```text
state=<状态值>;detail=<描述>
```

| state | 含义 | 常见 detail |
|---:|---|---|
| 0 | 未连接 | `platform_offline` |
| 1 | 正在连接 | `connecting` |
| 2 | 已连接 | `platform_connected` |
| 3 | 连接失败 | `platform_connect_failed`、`dispatch_failed` |

## 11. 异步响应模型

`0x0102`、`0x0105`、`0x0201` 会使用相同 command、sequence 返回两次：

| 阶段 | Status | Data | 含义 |
|---|---:|---|---|
| 受理 | `0x0000` | 空 | 已交给业务模块，尚未完成 |
| 最终成功 | `0x0000` | 状态文本或扫描 JSON | 操作完成 |
| 最终失败 | 通常 `0x0008` | 错误状态文本或空 | 操作失败 |

```text
CREATED → SENT → ACCEPTED → SUCCEEDED
                         ↘ FAILED
                         ↘ TIMED_OUT → QUERY_STATUS
```

建议超时：

| 操作 | 建议超时 |
|---|---:|
| 普通同步指令 | 3～5 秒 |
| 认证步骤 | 5 秒 |
| Wi-Fi 扫描 | 15～30 秒 |
| Wi-Fi 连接 | 30～60 秒 |
| 平台连接 | 30～60 秒 |

最终响应超时后先查询状态，不要立即重复提交密码或平台配置。

## 12. GATT 分片与流控

### 12.1 MTU、分片长度和协议帧长度

这三个长度不能混淆：

| 名称 | 含义 | 获取方式 |
|---|---|---|
| ATT MTU | 一次 ATT 报文允许的总长度 | Android 的 MTU 回调；iOS 由系统管理 |
| 单次特征值最大写入长度 | App 一次可以写入 Command 的业务字节数 | Android 通常为 `ATT_MTU - 3`；iOS 必须调用系统 API |
| 完整协议帧长度 | Header、Payload、CRC 合计长度 | `12 + Payload Length`，与 MTU 无关 |

- 最大协议 Payload 为 1024 字节，最大完整协议帧为 1036 字节。
- ATT MTU 默认值为 23 时，一次 Write/Notify 通常最多承载 20 字节特征值。
- Android 建议请求 MTU 185 或更大，但只能使用 `onMtuChanged` 最终成功返回的 MTU；请求值不代表协商值。
- iOS 不要假定 MTU，使用 `maximumWriteValueLength(for:)` 获取当前最大写入长度。
- 如果完整帧长度小于或等于最大写入长度，可以一次写完；否则必须由 App 分片写入。

设备接收请求时不依赖 App 的 MTU，也不要求 App 告知“总分片数”。设备把每次 `WriteValue` 的字节依次追加到缓存，缓存达到 10 字节后读取协议头中的 `Payload Length`：

```text
expectedFrameLength = 10 + payloadLength + 2
                    = 12 + payloadLength
```

当缓存达到 `expectedFrameLength` 后，设备取出完整帧并校验 CRC16。因此，同一条 48 字节请求既可以按 `20 + 20 + 8` 发送，也可以在最大写入长度足够时一次发送 48 字节。

BlueZ 通常会把协商 MTU 放在服务端 `WriteValue options["mtu"]` 中。设备会记录该值，用于把响应拆成不超过 `MTU - 3` 的 Notify；即使该字段缺失，也不影响设备按协议长度接收 App 请求，响应端会安全回退到 MTU 23、单片 20 字节。

### 12.2 App 请求分片规则

1. 先生成一条完整协议帧：`Header || Payload || CRC16`。
2. 再按照手机系统给出的最大写入长度，从头到尾连续切片。
3. 分片中不得增加分片序号、额外长度、帧头或 CRC。
4. 同一完整帧的分片必须严格按顺序写入。
5. 一条帧发送完成前，禁止发送另一条帧；多个业务请求必须进入同一个串行发送队列。
6. 推荐使用 Write With Response，每一片收到成功回调后才发送下一片。
7. 不建议配网流程使用 Write Without Response；如必须使用，需要遵守系统发送流控，并自行承担缺少逐片 ATT 确认的风险。
8. 从第一片开始到设备收齐完整帧，任意相邻写入不要接近 5 秒超时，建议控制在 1 秒以内。

```text
fullFrame = header || payload || crc
chunks    = split(fullFrame, maximumWriteLength)

for chunk in chunks:
    writeWithResponse(CommandCharacteristic, chunk)
    require writeSuccessCallback
```

发送示例：

```text
完整请求长度 = 48 字节

ATT MTU = 23，最大写入长度 = 20：
Write #1 = frame[0 .. 19]   20字节
Write #2 = frame[20 .. 39]  20字节
Write #3 = frame[40 .. 47]   8字节

ATT MTU = 185，最大写入长度 = 182：
Write #1 = frame[0 .. 47]   48字节
```

### 12.3 Android 发送状态机

以下是逻辑伪代码，具体 API 根据项目支持的 Android SDK 版本调整：

```kotlin
fun sendFrame(fullFrame: ByteArray) {
    require(state == IDLE)
    val maxWriteLength = negotiatedMtu - 3
    require(maxWriteLength > 0)

    txChunks = fullFrame.toList()
        .chunked(maxWriteLength)
        .map { it.toByteArray() }
    txIndex = 0
    state = WRITING
    writeNextChunkWithResponse()
}

fun onCharacteristicWrite(status: Int) {
    if (state != WRITING) return
    if (status != GATT_SUCCESS) {
        abortCurrentFrameAndRecover()
        return
    }

    txIndex++
    if (txIndex < txChunks.size) {
        writeNextChunkWithResponse()
    } else {
        state = WAITING_PROTOCOL_RESPONSE
        startResponseTimeout()
    }
}
```

不要在 `for` 循环中连续调用多次异步 `writeCharacteristic()` 而不等待回调；这可能造成 `GATT_BUSY`、覆盖待发送值或分片丢失。

### 12.4 iOS 发送状态机

```swift
let maxLength = peripheral.maximumWriteValueLength(for: .withResponse)
let chunks = split(fullFrame, maxLength)

// 每次只发一片；在 didWriteValueFor 且 error == nil 后发送下一片。
peripheral.writeValue(chunks[index],
                      for: commandCharacteristic,
                      type: .withResponse)
```

如果使用 `.withoutResponse`，必须在 `peripheralIsReady(toSendWriteWithoutResponse:)` 后继续发送，并检查 `canSendWriteWithoutResponse`。配网命令仍优先选择 `.withResponse`。

### 12.5 设备响应分片与 App 组帧

设备先生成完整响应帧，再按服务端获知的 `ATT_MTU - 3` 依次发送 Notify。App 不会在每个分片中看到额外分片头，必须把收到的原始字节追加到连接级接收缓存。

App 必须遵守以下规则：

- 不能假设一次 Notify 等于一条完整响应。
- 不能对单个 Notify 分片校验协议 CRC。
- 缓存达到 10 字节后读取 `Payload Length`，完整长度为 `12 + Payload Length`。
- 缓存不足完整长度时继续等待；达到完整长度后取出一帧并校验 CRC。
- 缓存中可能连续存在多条完整帧，解析成功后必须循环处理剩余数据。
- 如果开头不是 `54 56`，逐字节丢弃直到重新找到帧头。
- 声明的 Payload Length 大于 1024、CRC 错误或组帧超时时，丢弃坏帧并重新同步。
- BLE 断开、停止 Notify 或重新连接时，立即清空 App 本地接收缓存。

设备响应的重组示例：

```text
假设 ATT_MTU = 23，一条 54 字节响应会收到：
Notify #1：完整帧字节 [0 .. 19]    （20字节）
Notify #2：完整帧字节 [20 .. 39]   （20字节）
Notify #3：完整帧字节 [40 .. 53]   （14字节）

App 合并三片得到 54 字节后：
expectedLength = 12 + readU16BE(buffer, 8)
仅当 buffer.length >= expectedLength 时取出并校验完整帧。
```

建议的 App 接收器伪代码：

```text
onNotify(fragment):
    rxBuffer.append(fragment)

    while rxBuffer.length >= 10:
        if rxBuffer[0..1] != 54 56:
            rxBuffer.removeFirstByte()
            continue

        payloadLength = readU16BE(rxBuffer, 8)
        if payloadLength > 1024:
            rxBuffer.removeFirstByte()
            continue

        frameLength = 12 + payloadLength
        if rxBuffer.length < frameLength:
            startOrRefreshAssemblyTimeout()
            return

        frame = rxBuffer.removeFirst(frameLength)
        if crc16(frame[0 .. frameLength-3]) != readU16BE(frame, frameLength-2):
            reportBadFrameAndResynchronize()
            continue

        dispatchByCommandAndSequence(frame)
```

### 12.6 丢包、写失败与残包恢复

- BLE 链路层会处理无线链路上的校验与重传，但连接中断、App 未及时订阅或进程退出仍可能使业务响应不可达。
- CRC16 和总长度用于发现“不完整或损坏”，不能代替业务确认。
- Write With Response 可以确认每个分片是否被 ATT 层接受；它仍不能替代完整协议响应。
- 当前协议没有单独的“分片序号”和“重发第 N 片”命令，因此任何一片写失败后都不能只重发该片，也不能立即把完整帧第一片追加到设备残包后面。
- 分片写失败时，App 必须终止当前帧并选择一种恢复方式：等待超过 5 秒让设备接收缓存超时，再用新 sequence 从第一片重发；或者断开 BLE 后重连、重新订阅并重新认证。
- 设备在下一次写入到来时发现旧残包已超过 5 秒，会先清除残包，再把这次写入作为新帧第一片处理。
- 设备停止 Notify、BLE 服务退出时也会清除接收残包、认证会话及暂存敏感数据。
- App 等待响应超时后，可 Read Response Characteristic 读取设备缓存的最后一条完整响应；长响应由系统按 offset 继续读取。
- 如果 Read 得到的 `(command, sequence)` 不是当前等待的请求，不能误当成本次响应；应按对应指令的状态查询流程恢复。
- 对必须确认的请求，优先使用 Write With Response；业务响应仍以 Response Notify/Read 中的完整协议帧为准。

```text
某片写失败
    ↓
停止发送剩余分片
    ↓
清空 App 当前发送任务和响应组帧缓存
    ↓
方案A：等待 >5秒 → 使用新 sequence 从完整帧第一片重发
方案B：断开重连 → 重新订阅 → 重新认证 → 重新构造并发送请求
```

## 13. App 参考架构

```text
BleTransport
  ├─ 扫描、连接、服务发现、Notify订阅
  ├─ 串行发送队列、MTU分片写入
  └─ Notify接收缓存、超时清理

FrameCodec
  ├─ 大端整数、CRC16
  ├─ 请求编码、响应解析
  └─ 按Payload Length连续组帧和帧头重同步

ProvisioningSecurity
  ├─ 二维码解析、HMAC认证
  ├─ sessionKey派生、counter管理
  └─ AES-256-GCM

ProvisioningClient
  ├─ sequence管理、请求/响应关联
  ├─ 异步双响应状态机
  └─ Wi-Fi/平台业务流程
```

发送安全指令：

```text
sendSecureCommand(command, plainData):
    require authenticated
    sequence      = nextSequence()
    securePayload = encryptBusinessPayload(command, sequence, plainData)
    frame         = encodeFrame(1, 0, command, sequence, securePayload)
    pending[(command, sequence)] = new RequestState()
    writeFrameInChunks(frame)
```

## 14. 错误恢复和重连

| 场景 | 建议处理 |
|---|---|
| BLE 意外断开 | 清除 sessionKey、sessionId、counter 和 pending 请求；重连后重新认证 |
| `STATUS_UNAUTHORIZED` | 清除本地会话并从 `0x0010` 重新认证 |
| `STATUS_AUTH_FAILED` | 检查二维码密钥、HMAC/AAD/Tag；不要盲目连续重试 |
| `STATUS_AUTH_LOCKED` | 至少等待 60 秒后重新认证 |
| `STATUS_REPLAY_DETECTED` | 用新 counter、nonce、sequence 重新加密 |
| 异步最终响应超时 | 发送状态查询；确认失败后再创建新请求 |
| 平台成功后 BLE 断开 | 正常完成，不要自动重连 |
| 收到未知 sequence | 记录并忽略，不要完成其他 pending 请求 |
| Notify 组帧超过 5 秒仍不完整 | 清空 App 接收缓存；Read Response 尝试恢复最后响应，失败后按状态查询或以新 sequence 重发 |
| CRC 或声明长度不正确 | 丢弃当前坏帧并重新搜索 `54 56` 帧头，不能把残留字节拼到新请求 |

设备停止 Notify 或服务退出时会销毁认证会话并清空未完成的请求分片。App 不能在重新连接后继续使用旧 sessionKey、counter 或旧接收缓存。

## 15. 安全要求

- CRC 只用于发现传输错误，不提供身份认证或保密性。
- 设备身份和 App 授权依赖二维码中的设备独立 setupSecret。
- Wi-Fi 和平台业务请求使用 HMAC-SHA256、派生会话密钥及 AES-256-GCM。
- AES-GCM nonce 在同一 sessionKey 下必须唯一。
- 不允许不同设备共用 setupSecret。
- 不允许在日志、崩溃报告、埋点或网络请求中上传任何密钥和密码。
- 二维码应只提供给设备所有者；设备转移或二维码泄露时应提供重新生成 setupSecret 的维护流程。
- BLE 广播和连接仍可能被附近手机发现；二维码认证保护敏感指令权限，而不是使设备“不可被扫描”。

## 16. 联调检查清单

### 16.1 基础链路

- [ ] 按 Service UUID 扫描到目标设备。
- [ ] Local Name 与二维码的 model、sn 一致。
- [ ] 发现 Command 和 Response 特征。
- [ ] Response Notify 订阅成功。
- [ ] `0x0001` 返回协议与安全能力。
- [ ] `0x0002` 原样返回心跳 Data。

### 16.2 认证和加密

- [ ] appNonce 使用密码学安全随机数生成器。
- [ ] sessionId 按大端序参与 HMAC 和安全 Payload。
- [ ] HMAC 输入字节顺序完全正确。
- [ ] sessionKey 派生末尾是单字节 `0x01`，不是字符 `'1'`。
- [ ] counter 从 1 开始严格递增。
- [ ] nonce 为 12 字节且不重复。
- [ ] AAD 字段顺序和字节序正确。
- [ ] GCM Tag 长度为 16 字节。
- [ ] 重连后不复用旧会话。

### 16.3 配网业务

- [ ] 解析扫描 JSON 时只依赖 `ssid`、`security`。
- [ ] JSON 序列化和解析统一使用 UTF-8，不携带 BOM。
- [ ] 使用中文 SSID 完成扫描、显示、选择、保存凭据和连接全流程。
- [ ] App 按 UTF-8 字节数验证 SSID 为 1～32字节，而不是按字符数验证。
- [ ] 中文 UTF-8 字节跨 BLE 分片时，App 不逐片转字符串，设备仍能正确连接。
- [ ] 测试直接 UTF-8 和标准 `\uXXXX` 两种 JSON 表示，解析结果一致。
- [ ] 拒绝超过32个 UTF-8 字节、包含 NUL 或编码非法的 SSID。
- [ ] 区分受理响应和最终响应。
- [ ] Wi-Fi 最终确认 `state=3`。
- [ ] Wi-Fi 成功后才发送平台连接命令。
- [ ] 平台最终确认 `state=2`。
- [ ] 平台成功后 BLE 主动断开被识别为正常完成。

### 16.4 分片和异常恢复

- [ ] 使用 MTU=23 验证大于 20 字节的请求能够分片发送。
- [ ] 每片 Write With Response 成功后才发送下一片。
- [ ] 验证两条请求的分片不会交错。
- [ ] 验证 Wi-Fi 扫描长响应可以由多个 Notify 组装并通过 CRC。
- [ ] 验证同一 Notify 缓存中包含两条完整帧时可以循环解析。
- [ ] 人为中止请求分片，等待超过 5 秒后发送新帧，确认设备清除旧残包。
- [ ] 人为制造 CRC 错误，确认 App 丢弃坏帧且后续正常帧仍能解析。
- [ ] BLE 断开重连后，App 清空收发缓存并重新认证。

### 16.5 日志脱敏

- [ ] 不打印 setupSecret。
- [ ] 不打印 sessionKey、Proof、完整 nonce/tag 或完整密文。
- [ ] 不打印 Wi-Fi 和平台密码。
- [ ] 调试日志仅包含 command、sequence、status、长度和流程状态。

## 17. 常见问题

### Q1：为什么空业务 Data 的安全 Payload 仍有 36 字节？

因为仍需携带 sessionId、counter、nonce 和 GCM Tag；只有 ciphertext 长度为 0。

### Q2：第一条 `STATUS_OK` 是否表示 Wi-Fi 已连接？

不是。第一条空 Data 响应只表示已受理，必须等待最终响应或查询到 Wi-Fi `state=3`。

### Q3：为什么返回 `STATUS_REPLAY_DETECTED`？

counter 已被使用或小于设备记录值。请递增 counter、生成新 nonce，并重新加密整个请求。

### Q4：为什么能连接 BLE，却不能发送配网指令？

BLE 连接不等于协议认证。必须使用二维码 setupSecret 完成 `0x0010`、`0x0011`，再发送加密业务指令。

### Q5：App 是否必须在应用层分片？

当完整协议帧超过手机系统报告的单次最大写入长度时必须分片。手机 BLE 协议栈不会自动把任意长度的特征值写入转换成设备当前协议所需的多次 `WriteValue`；超长写入可能被拒绝、截断或返回失败。App 必须先生成完整帧，再按最大写入长度切片并串行发送。

### Q6：设备不知道 App 的分片大小，如何判断已经收完？

设备不依赖分片大小和分片数量。它连续累计字节，在收到 10 字节固定头后读取 `Payload Length`，由 `12 + Payload Length` 得到完整帧长度，收齐后再校验 CRC16。MTU 只决定每次传多少字节，不决定协议帧边界。

### Q7：某一片发送失败后能否只重发这一片？

不能。当前协议没有独立分片序号。App 应停止当前帧，等待设备残包超过 5 秒被清理后使用新 sequence 从第一片重发；或者断开重连、重新订阅和认证后重发。不能把第一片立即追加在设备已有残包之后。

### Q8：平台成功后 BLE 突然断开是否异常？

不是。设备确认 Wi-Fi 和平台均连接后会主动关闭 BLE 配网服务。

### Q9：Wi-Fi 扫描结果为什么没有信号强度和频段？

当前接口只返回 `ssid` 和 `security`，不返回 `rssi`、`band`、`current`。

### Q10：是否支持中文 Wi-Fi 名称？

支持。App 以 UTF-8 JSON提交 SSID，BLE 和加密层按原始字节传输，设备 JSON解析后会把非 ASCII SSID 的 UTF-8字节转换成 `wpa_supplicant` 所需的十六进制形式。SSID 必须是合法 UTF-8，且 UTF-8编码结果不能超过32字节。

### Q11：中文字符被拆到两个 BLE 分片中会不会乱码？

不会。协议分片处理的是二进制字节，不在分片层解析字符。接收方必须先按协议长度组装完整帧，再校验 CRC、解密并进行一次 UTF-8解码。只有错误地逐片转字符串才可能产生乱码。
