#include "ble_command_protocol.h"

#include <algorithm>
#include <chrono>

namespace BleProvisioning
{
namespace
{
/* 从网络字节序缓冲区读取一个16位无符号整数。 */
uint16_t readU16(const uint8_t *p)
{
    return static_cast<uint16_t>((static_cast<uint16_t>(p[0]) << 8) | p[1]);
}

/* 把16位无符号整数按网络字节序追加到字节数组。 */
void appendU16(std::vector<uint8_t> &out, uint16_t value)
{
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xff));
    out.push_back(static_cast<uint8_t>(value & 0xff));
}

/* 返回不受系统时间修改影响的单调毫秒时间，用于残包超时判断。 */
std::uint64_t monotonicMilliseconds()
{
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
}
} // namespace

uint16_t FrameCodec::crc16(const uint8_t *data, std::size_t size)
{
    /* CRC16-CCITT-FALSE：初值 0xffff，多项式 0x1021，不反转。 */
    uint16_t crc = 0xffff;
    for (std::size_t i = 0; i < size; ++i)
    {
        crc ^= static_cast<uint16_t>(data[i]) << 8;
        for (int bit = 0; bit < 8; ++bit)
            crc = (crc & 0x8000) ? static_cast<uint16_t>((crc << 1) ^ 0x1021)
                                 : static_cast<uint16_t>(crc << 1);
    }
    return crc;
}

std::vector<uint8_t> FrameCodec::encode(const CommandFrame &frame)
{
    if (frame.payload.size() > kMaxPayloadSize)
        return std::vector<uint8_t>();

    std::vector<uint8_t> out;
    out.reserve(kFrameHeaderSize + frame.payload.size() + kFrameCrcSize);
    out.push_back(0x54);
    out.push_back(0x56);
    out.push_back(frame.version);
    out.push_back(frame.flags);
    appendU16(out, frame.command);
    appendU16(out, frame.sequence);
    appendU16(out, static_cast<uint16_t>(frame.payload.size()));
    out.insert(out.end(), frame.payload.begin(), frame.payload.end());
    /* CRC 覆盖 magic 到 payload 的全部内容，CRC 字段自身不参与计算。 */
    appendU16(out, crc16(out.data(), out.size()));
    return out;
}

bool FrameCodec::decode(const uint8_t *data, std::size_t size,
                        CommandFrame &frame, StatusCode &status)
{
    status = STATUS_BAD_FRAME;
    if (!data || size < kFrameHeaderSize + kFrameCrcSize ||
        data[0] != 0x54 || data[1] != 0x56)
        return false;

    if (data[2] != kProtocolVersion)
    {
        status = STATUS_UNSUPPORTED_VERSION;
        return false;
    }

    const uint16_t payloadSize = readU16(data + 8);
    if (payloadSize > kMaxPayloadSize ||
        size != kFrameHeaderSize + payloadSize + kFrameCrcSize)
        return false;

    const uint16_t expectedCrc = readU16(data + size - kFrameCrcSize);
    if (crc16(data, size - kFrameCrcSize) != expectedCrc)
    {
        status = STATUS_BAD_CRC;
        return false;
    }

    frame.version = data[2];
    frame.flags = data[3];
    frame.command = readU16(data + 4);
    frame.sequence = readU16(data + 6);
    frame.payload.assign(data + kFrameHeaderSize,
                         data + kFrameHeaderSize + payloadSize);
    status = STATUS_OK;
    return true;
}

void FrameDecoder::append(const uint8_t *data, std::size_t size,
                          const FrameCallback &onFrame,
                          const ErrorCallback &onError)
{
    if (!data || size == 0)
        return;

    const std::uint64_t now = monotonicMilliseconds();
    if (!m_buffer.empty() && m_lastAppendMs != 0 &&
        now - m_lastAppendMs > kFrameAssemblyTimeoutMs)
    {
        /* 上一帧长期未收齐。先清残包，再把本次写入当作新帧开始处理。 */
        reset();
        if (onError)
            onError(STATUS_BAD_FRAME);
    }
    m_lastAppendMs = now;

    /*
     * 当前协议最多缓存一条完整帧。超过上限通常意味着分片丢失、顺序错误
     * 或恶意超长输入，直接清空可避免缓存无限增长。
     */
    if (m_buffer.size() + size > kFrameHeaderSize + kMaxPayloadSize + kFrameCrcSize)
    {
        const std::size_t staleSize = m_buffer.size();
        reset();
        if (onError)
            onError(STATUS_BAD_FRAME);

        /*
         * 缓存溢出通常由旧残包造成。如果本次写入本身不超限且以帧头开始，
         * 保留它作为新帧首片，避免旧数据破坏新的合法请求。
         */
        if (staleSize != 0 &&
            size <= kFrameHeaderSize + kMaxPayloadSize + kFrameCrcSize &&
            size >= 2 && data[0] == 0x54 && data[1] == 0x56)
        {
            m_buffer.insert(m_buffer.end(), data, data + size);
            m_lastAppendMs = now;
        }
        else
        {
            return;
        }
    }
    else
    {
        m_buffer.insert(m_buffer.end(), data, data + size);
    }

    bool badFrameReported = false;
    while (m_buffer.size() >= kFrameHeaderSize)
    {
        if (m_buffer[0] != 0x54 || m_buffer[1] != 0x56)
        {
            /* 逐字节重新同步帧头，兼容粘包和异常数据。 */
            m_buffer.erase(m_buffer.begin());
            if (onError && !badFrameReported)
            {
                onError(STATUS_BAD_FRAME);
                badFrameReported = true;
            }
            continue;
        }

        const uint16_t payloadSize = readU16(&m_buffer[8]);
        if (payloadSize > kMaxPayloadSize)
        {
            if (onError)
                onError(STATUS_BAD_FRAME);
            /* 长度字段损坏时逐字节重同步，保留后面可能已经到达的合法帧。 */
            m_buffer.erase(m_buffer.begin());
            badFrameReported = true;
            continue;
        }

        /* 先从固定头取得 payload 长度，数据不完整时保留缓存等待下个分片。 */
        const std::size_t frameSize = kFrameHeaderSize + payloadSize + kFrameCrcSize;
        if (m_buffer.size() < frameSize)
            return;

        CommandFrame frame;
        StatusCode status;
        if (FrameCodec::decode(m_buffer.data(), frameSize, frame, status))
        {
            m_buffer.erase(m_buffer.begin(), m_buffer.begin() + frameSize);
            if (onFrame)
                onFrame(frame);
        }
        else
        {
            if (onError)
                onError(status);
            /*
             * CRC或版本错误时只丢弃一个字节并重新搜索帧头。不能直接删除
             * frameSize 字节，否则残包拼上下一帧后可能把下一帧前部一起吞掉。
             */
            m_buffer.erase(m_buffer.begin());
            badFrameReported = true;
        }
    }

    if (m_buffer.empty())
        m_lastAppendMs = 0;
}

void FrameDecoder::reset()
{
    m_buffer.clear();
    m_lastAppendMs = 0;
}

std::size_t FrameDecoder::bufferedSize() const
{
    return m_buffer.size();
}

CommandDispatcher::CommandDispatcher()
{
    registerHandler(CMD_GET_PROTOCOL_INFO, [](const CommandFrame &) {
        CommandResult result;
        const char info[] =
            "TV-BLE/1;frame=crc16;max=1024;auth=hmac-sha256;cipher=aes-256-gcm";
        result.payload.assign(info, info + sizeof(info) - 1);
        return result;
    });
    registerHandler(CMD_HEARTBEAT, [](const CommandFrame &frame) {
        CommandResult result;
        result.payload = frame.payload;
        return result;
    });
}

void CommandDispatcher::registerHandler(uint16_t command, const Handler &handler)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (handler)
        m_handlers[command] = handler;
    else
        m_handlers.erase(command);
}

CommandResult CommandDispatcher::dispatch(const CommandFrame &frame) const
{
    Handler handler;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::map<uint16_t, Handler>::const_iterator it = m_handlers.find(frame.command);
        if (it == m_handlers.end())
        {
            CommandResult result;
            result.status = STATUS_UNKNOWN_COMMAND;
            return result;
        }
        /* 复制回调后释放锁，业务 Handler 可以安全地再次操作分发表。 */
        handler = it->second;
    }

    try
    {
        return handler(frame);
    }
    catch (...)
    {
        CommandResult result;
        result.status = STATUS_INTERNAL_ERROR;
        return result;
    }
}

std::vector<uint8_t> makeResponsePayload(StatusCode status,
                                         const std::vector<uint8_t> &data)
{
    std::vector<uint8_t> payload;
    payload.reserve(data.size() + 2);
    /* 所有响应 payload 的前两个字节固定为大端状态码，后面才是业务数据。 */
    appendU16(payload, static_cast<uint16_t>(status));
    payload.insert(payload.end(), data.begin(), data.end());
    return payload;
}

} // namespace BleProvisioning
