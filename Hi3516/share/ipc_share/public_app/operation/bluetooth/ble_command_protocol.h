#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace BleProvisioning
{

/*
 * BLE 指令帧（网络字节序）：
 *   magic[2]  = 0x54, 0x56 ('T','V')
 *   version   = 1
 *   flags     = bit0: response
 *   command   = uint16
 *   sequence  = uint16
 *   length    = uint16，payload长度
 *   payload   = 0..1024字节
 *   crc16     = CRC16-CCITT，覆盖crc之前的所有字节
 *
 * 同一帧允许由多次 GATT WriteValue 顺序写入，FrameDecoder 负责组帧。
 */
static const uint8_t kProtocolVersion = 1;
static const std::size_t kFrameHeaderSize = 10;
static const std::size_t kFrameCrcSize = 2;
static const std::size_t kMaxPayloadSize = 1024;
/* 首片到后续片间隔超过5秒，认为上一帧已经丢失，下一次写入前清理残包。 */
static const std::uint64_t kFrameAssemblyTimeoutMs = 5000;

enum CommandCode : uint16_t
{
    /* 基础能力：无需认证，用于协议探测和链路保活。 */
    CMD_GET_PROTOCOL_INFO = 0x0001,
    CMD_HEARTBEAT = 0x0002,

    /* 安全握手：使用设备二维码中的 setupSecret 建立临时加密会话。 */
    CMD_AUTH_BEGIN = 0x0010,
    CMD_AUTH_PROVE = 0x0011,
    CMD_AUTH_GET_STATUS = 0x0012,

    /* Wi-Fi 配网：这些指令的 payload 必须经过 AES-256-GCM 加密。 */
    CMD_WIFI_SET_CREDENTIALS = 0x0101,
    CMD_WIFI_CONNECT = 0x0102,
    CMD_WIFI_GET_STATUS = 0x0103,
    CMD_WIFI_CANCEL = 0x0104,
    CMD_WIFI_SCAN = 0x0105,

    /* 平台接入：Wi-Fi 可用后再连接平台，同样要求安全会话。 */
    CMD_PLATFORM_CONNECT = 0x0201,
    CMD_PLATFORM_GET_STATUS = 0x0202,

    /* 0x0300-0x6fff：后续设备功能；0x7000-0x7fff：厂商扩展。 */
};

enum StatusCode : uint16_t
{
    STATUS_OK = 0,
    STATUS_BAD_FRAME = 1,
    STATUS_BAD_CRC = 2,
    STATUS_UNSUPPORTED_VERSION = 3,
    STATUS_UNKNOWN_COMMAND = 4,
    STATUS_INVALID_PAYLOAD = 5,
    STATUS_BUSY = 6,
    STATUS_NOT_IMPLEMENTED = 7,
    STATUS_INTERNAL_ERROR = 8,
    STATUS_UNAUTHORIZED = 9,
    STATUS_AUTH_FAILED = 10,
    STATUS_AUTH_LOCKED = 11,
    STATUS_REPLAY_DETECTED = 12
};

struct CommandFrame
{
    /*
     * sequence 由 App 生成，设备响应时原样带回，便于匹配请求和响应。
     * 它只负责请求关联，不承担防重放；防重放由加密载荷中的 counter 完成。
     */
    uint8_t version = kProtocolVersion;
    uint8_t flags = 0;
    uint16_t command = 0;
    uint16_t sequence = 0;
    std::vector<uint8_t> payload;
};

struct CommandResult
{
    StatusCode status = STATUS_OK;
    std::vector<uint8_t> payload;
};

class FrameCodec
{
public:
    /** 计算 CRC16-CCITT-FALSE，用于校验帧头、Payload 的完整性。 */
    static uint16_t crc16(const uint8_t *data, std::size_t size);
    /** 将字段结构编码为可通过 GATT 传输的完整二进制帧；超长时返回空数组。 */
    static std::vector<uint8_t> encode(const CommandFrame &frame);
    /**
     * 校验并解析一条长度完全匹配的完整帧。
     * @param data 完整帧首地址。
     * @param size 完整帧字节数。
     * @param frame 成功时输出解析结果。
     * @param status 失败时输出具体协议状态码。
     */
    static bool decode(const uint8_t *data, std::size_t size,
                       CommandFrame &frame, StatusCode &status);
};

class FrameDecoder
{
public:
    typedef std::function<void(const CommandFrame &)> FrameCallback;
    typedef std::function<void(StatusCode)> ErrorCallback;

    /**
     * GATT 单次写入受 MTU 限制，一条协议帧可能被拆成多个 WriteValue。
     * append() 将顺序到达的原始字节追加到缓存，收齐后才校验并回调。
     * 同时处理粘包、错误帧头、超长数据和5秒残包超时。
     */
    void append(const uint8_t *data, std::size_t size,
                const FrameCallback &onFrame, const ErrorCallback &onError);
    /** 丢弃当前未完成帧；连接切换、写偏移错误或会话结束时调用。 */
    void reset();
    /** 返回当前尚未组成完整帧的缓存字节数，仅用于状态判断和调试日志。 */
    std::size_t bufferedSize() const;

private:
    std::vector<uint8_t> m_buffer;
    std::uint64_t m_lastAppendMs = 0;
};

class CommandDispatcher
{
public:
    typedef std::function<CommandResult(const CommandFrame &)> Handler;

    /** 创建分发器，并注册协议查询和心跳两个基础指令。 */
    CommandDispatcher();
    /** 为指令码注册处理函数；传入空 Handler 表示删除已有处理函数。 */
    void registerHandler(uint16_t command, const Handler &handler);
    /** 查找并执行指令处理函数；未知指令或异常会转换为统一状态码。 */
    CommandResult dispatch(const CommandFrame &frame) const;

private:
    mutable std::mutex m_mutex;
    std::map<uint16_t, Handler> m_handlers;
};

/** 在业务 Data 前增加2字节大端状态码，生成统一的响应 Payload。 */
std::vector<uint8_t> makeResponsePayload(StatusCode status,
                                         const std::vector<uint8_t> &data);

} // namespace BleProvisioning
