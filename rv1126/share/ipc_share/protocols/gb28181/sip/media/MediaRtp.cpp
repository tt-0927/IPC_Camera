#include "MediaRtp.h"
#include "MediaPs.h"
#include "dlog.h"
#include <iomanip> // 包含用于设置输出格式的函数
#include <iostream>
#include <sstream>
#include <sys/time.h>

using namespace SIP;
using namespace RTP;

#define RTP_DEBUG 0
#define RTP_DEBUG_FILE 0
#define RTP_DEBUG_RTP_FILE 0
#define RTP_DEBUG_PS_FILE 0
#define RTP_DEBUG_H264_FILE 0

SIP::RTP::Parser::Parser(uint32_t ssrc, RtpMediaDataCb fnCb)
    : m_ssrc(ssrc), m_fnMediaCb(fnCb), buff_timestamp_(0)
{
#if RTP_DEBUG_FILE
#if RTP_DEBUG_RTP_FILE
    m_rtpfile.open("./test.rtp", std::ios::out | std::ios::binary | std::ios::trunc);
#endif
#if RTP_DEBUG_PS_FILE
    m_psfile.open("./test.ps", std::ios::out | std::ios::binary | std::ios::trunc);
#endif
#endif
}

SIP::RTP::Parser::~Parser()
{
#if RTP_DEBUG_FILE
#if RTP_DEBUG_RTP_FILE
    if (m_rtpfile.is_open())
    {
        m_rtpfile.close();
    }
#endif
#if RTP_DEBUG_PS_FILE
    if (m_psfile.is_open())
    {
        m_psfile.close();
    }
#endif
#endif
}

void Parser::reset(uint32_t ssrc)
{
    m_ssrc = ssrc;
    buffer_.clear();
}

bool Parser::parsePacket(
    const char *data,
    size_t size,bool bAudio)
{
    /** RTP 头部长度至少 12 字节，否则无效 */
    if (size < 12)
    {
        dlog_error("Invalid RTP packet size: %zu", size);
        return false;
    }

    RtpHeader header;
    bool bParseSuccess = false;
    size_t offset = 0;

    const char *ptr = data;
#if RTP_DEBUG
    { /* DEBUG */
        std::cout << "数据头:"
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[0])
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[1])
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[2])
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[3])
                  << " "
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[4])
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[5])
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[6])
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[7])
                  << " "
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[8])
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[9])
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[10])
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[11])
                  << " "
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[12])
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[13])
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[14])
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[15])
                  << " "
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[16])
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[17])
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[18])
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[19])
                  << " "
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[20])
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[21])
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[22])
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[23])
                  << " "
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[24])
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[25])
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[26])
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[27])
                  << " "
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[28])
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[29])
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[30])
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[31])
                  << " "
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[32])
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[33])
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[34])
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[35])
                  << " "
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[36])
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[37])
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[38])
                  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ptr[39])
                  << " "
                  << std::endl;
    }
#endif
    /** 解析版本号，取前 2 位 */
    header.version = (ptr[0] >> 6) & 0x03;
    /** 解析填充标志，取第 5 位 */
    header.padding = (ptr[0] >> 5) & 0x01;
    /** 解析扩展标志，取第 4 位 */
    header.extension = (ptr[0] >> 4) & 0x01;
    /** 解析 CSRC 计数，取后 4 位 */
    header.csrcCount = ptr[0] & 0x0F;
    /** 解析标志位，取第 8 位 */
    header.marker = (ptr[1] >> 7) & 0x01;
    /** 解析负载类型，取后 7 位 */
    header.payloadType = ptr[1] & 0x7F;
    /** 解析 16 位序列号，高 8 位在前，低 8 位在后 */
    header.sequenceNumber = (static_cast<uint16_t>(ptr[2]) << 8) | ptr[3];
    /** 解析 32 位时间戳，每 8 位依次拼接 */
    header.timestamp = (static_cast<uint32_t>(ptr[4]) << 24) | (static_cast<uint32_t>(ptr[5]) << 16) | (static_cast<uint32_t>(ptr[6]) << 8) | ptr[7];
    /** 解析 32 位 SSRC，每 8 位依次拼接 */
    header.ssrc = (static_cast<uint32_t>(ptr[8]) << 24) | (static_cast<uint32_t>(ptr[9]) << 16) | (static_cast<uint32_t>(ptr[10]) << 8) | ptr[11];

    /** 确保 RTP 包属于当前解析器处理的 SSRC */
    if (header.ssrc != m_ssrc)
    {
        dlog_error("解析RTP包的SSRC[0x%08x]与当前解析器SSRC[0x%08x]不匹配 RTP包大小[%d]",
                   header.ssrc, m_ssrc, size);
    }
    else
    {
        bParseSuccess = true;
    }

    if (!bParseSuccess)
    {
        return false;
    }
#if RTP_DEBUG
    dlog_debug("RTP packet: v[%d] p[%d] e[%d] csrc[%d] m[%d] payloadType[%d] sn[%d] ts[%ld] ssrc[%08x]",
               header.version, header.padding, header.extension, header.csrcCount, header.marker, header.payloadType, header.sequenceNumber, header.timestamp, header.ssrc);
#endif
    /** 计算 RTP 头部总长度（含 CSRC） */
    size_t headerSize = 12 + header.csrcCount * 4;
    if (size <= headerSize)
    {
        return false;
    }
    const char *pStart = data + offset + headerSize; /* 偏移量 */
    const char *pEnd = data + size;                  /* 末尾 */
    /** 提取负载数据 */
    std::vector<char> payload(pStart, pEnd);
    /* 记录当前RTP包的时间戳 */
    uint64_t nRtpTs;

    /* 组装完的数据 */
    std::vector<char> frame;

    /* 记录分包缓存中的时间戳，以防后续重组成功后时间戳更新 */
    nRtpTs = buff_timestamp_.load();
    /** 分包情况，缓存并尝试重组 */
    if (!processFragmentedPacket(
            header.sequenceNumber,
            header.timestamp,
            header.marker,
            payload,
            frame))
    {
        /* 同样返回true即可，虽然没有重组成功，但还算解析成功 */
        return true;
    }
#if RTP_DEBUG_FILE && RTP_DEBUG_PS_FILE
    if (m_psfile.is_open())
    {
        m_psfile.write(frame.data(), frame.size());
        m_psfile.flush();
    }
#endif
   
    if(!bAudio)
    {
        /* TODO 后续针对不同payload type进行解析 */
        ::PS::parse(frame, m_fnMediaCb, nRtpTs);
    }
     /* 广播音频数据不用ps */
    else
    {
        /* 上抛数据*/
        if (m_fnMediaCb)
        {

            SIP::RTP::MediaInfo_S stCbData;
            stCbData.pData = frame.data();
            stCbData.nLen = frame.size();
            stCbData.bIsAudio = true;
            stCbData.bIsKeyFrame = false; /* 默认填false */
            stCbData.nStreamType = 0;     /* 默认填0 */
            m_fnMediaCb(stCbData);
        }
    }
    return true;
}

bool Parser::parsePacketTcp(const char *data, size_t size)
{
/* NOTE 一个TCP包首先解析RTP包，沾包情况下，把无法拼接的数据缓存，等待下个TCP包一起拼接 */
#if RTP_DEBUG_FILE && RTP_DEBUG_RTP_FILE
    if (m_rtpfile.is_open())
    {
        m_rtpfile.write(data, size);
        m_rtpfile.flush();
    }
#endif
    auto vecTotal = m_vecTcpBuffer.size();
#if RTP_DEBUG
    dlog_debug("接收的TCP数据大小[%llu]，缓存数据大小[%llu]", size, vecTotal);
#endif
    size_t nOffset = 0;
    if (vecTotal > 0)
    {
        /* NOTE nOffset主要是计算偏移一个RTP包大小后的位置 */
        /* 需要用2字节获取RTP包大小 */
        size_t nRtpSize = 0;
        nRtpSize |= m_vecTcpBuffer[0] << 8;
        if (vecTotal == 1)
        {
            /* 缓存只有1字节时，需要用1字节获取RTP包大小 */
            nRtpSize |= data[0];
            /* 第一个字节用作RTP包的低8位，后续需要偏移整个nRtpSize */
            nOffset += nRtpSize + 1;
        }
        else
        {
            nRtpSize |= m_vecTcpBuffer[1];
            nOffset += nRtpSize - (vecTotal - 2);
        }

        m_vecTcpBuffer.insert(m_vecTcpBuffer.end(), data, data + nOffset);
#if RTP_DEBUG
        auto frameTotal = m_vecTcpBuffer.size();
        dlog_debug("解析缓存数据的Rtp数据包大小[%llu][%02x][%02x]需要拼接的大小[%llu]拼好包[%llu]",
                   nRtpSize, m_vecTcpBuffer[0], m_vecTcpBuffer[1], nOffset, frameTotal);
#endif
        if (!parsePacket(m_vecTcpBuffer.data() + 2, nRtpSize,false))
        {
#if RTP_DEBUG
            dlog_debug("无法解析拼好包[%llu]", frameTotal);
#endif
        }
        /* 清空缓存的数据 */
        m_vecTcpBuffer.clear();
    }
    const size_t nTotal = size;
    int nCount = 0;
    bool bRet = true;
    while (nOffset < nTotal)
    {
        /* 1、读取两个字节，获取RTP数据包长度 */
        if (nTotal - nOffset < 2)
        {
            /* 有可能剩下1个字节，连长度都没法解析出来 */
            bRet = false;
            break;
        }
        /* 分离沾包进行单包RTP数据解析 */

        /* 通过读取前两字节，获取RTP数据包 */
        size_t nRtpSize = 0;
        nRtpSize |= data[nOffset] << 8;
        nRtpSize |= data[nOffset + 1];
        size_t nLeftSize = nTotal - nOffset - 2;
#if RTP_DEBUG
        dlog_debug("解析TCP数据的Rtp数据包大小[%llu][%02x][%02x]剩余大小[%llu]Offset[%llu]Total[%llu]",
                   nRtpSize, data[nOffset], data[nOffset + 1], nLeftSize, nOffset, nTotal);
#endif
        /* nLeftSize可以为0，即只有长度数据，完全没有RTP包数据 */
        if (nLeftSize < nRtpSize)
        {
#if RTP_DEBUG
            dlog_debug("剩余的数据长度无法满足解析出来的RTP数据长度");
#endif
            bRet = false;
            break;
        }
        nOffset += 2; /* 偏移掉表示RTP数据包长度的两字节 */
        if (!parsePacket(data + nOffset, nRtpSize,false))
        {
#if RTP_DEBUG
            dlog_debug("无法解析对应RTP数据长度[%llu]", nRtpSize);
#endif
        }
        nCount++;
        nOffset += nRtpSize;
    }
    /* NOTE 可能会剩下1或2个字节，导致数据不足无法解析，缓存到下一次进行解析 */
    if (!bRet && (nTotal - nOffset) > 0)
    {
#if RTP_DEBUG
        dlog_debug("剩余[%llu]TCP数据无法解析", nTotal - nOffset);
#endif
        /* 剩下的数据包不完整，不解析，等待下一包数据拼接后再解析 */
        m_vecTcpBuffer.insert(m_vecTcpBuffer.end(), data + nOffset, data + nTotal);
    }
#if RTP_DEBUG
    dlog_debug("TCP数据[%llu]解析出[%d]个RTP包", size, nCount);
#endif
    return bRet;
}

bool Parser::processFragmentedPacket(
    uint16_t sequenceNumber,
    uint64_t timestamp,
    uint8_t marker,
    const std::vector<char> &payload,
    std::vector<char> &output)
{
    bool bRet = false;
    /* FIXME 使用时间戳变更来进行重组分片，会导致后续没有数据时，上一个时间戳缓存的分片无法进行重组 */
    /* 首次缓存或重置过时间戳则先更新一次时间戳 */
    if (0 == buff_timestamp_.load())
    {
        buff_timestamp_.store(timestamp);
    }
    /* NOTE （存在部分RTP流的marker标记位在分片结束时不会至为1的问题） */
    /* 时间戳不一致的时候需要重组，否则继续缓存 */
    if (timestamp != buff_timestamp_.load())
    {
#if RTP_DEBUG
        dlog_debug("RTP packet timestamp: %d => %d", buff_timestamp_.load(), timestamp);
#endif
        /* 组装上一个时间戳的分片数据 */
        bRet = makePacket(output);
        /* 组装完数据后，会把时间戳清空，更新时间戳 */
        buff_timestamp_.store(timestamp);
        /* 重新缓存当前新的时间戳的数据 */
        buffer_[sequenceNumber] = payload;
    }
    else
    {
        /* 时间戳一致则继续缓存 */
        buffer_[sequenceNumber] = payload;
        /* 再判断marker标记位 */
        if (marker)
        {
#if RTP_DEBUG
            /* 组装完整数据 */
            dlog_debug("RTP packet marker: %d", marker);
#endif
            bRet = makePacket(output);
        }
    }
    return bRet;
}

bool SIP::RTP::Parser::makePacket(std::vector<char> &output)
{

    int nDebugCount = 0;
    /** 组装完整数据 */
    std::vector<char> assembled;
    uint16_t expectedSeq = buffer_.begin()->first;
    /* 按顺序重组数据 */
    while (buffer_.count(expectedSeq))
    {
        assembled.insert(assembled.end(), buffer_[expectedSeq].begin(), buffer_[expectedSeq].end());
        buffer_.erase(expectedSeq);
        expectedSeq++;
        nDebugCount++;
    }
    /* 理论上，成功重组完后，buffer_ 应该为空，如果不为空，则有丢包 */
    if (!buffer_.empty())
    {
        dlog_error("RTP packet lost: %d", buffer_.begin()->first);
        buffer_.clear();
    }
#if RTP_DEBUG
    dlog_debug("Packet timestamp: %ld", buff_timestamp_.load());
#endif
    /* 组装完后数据时间戳要归零 */
    buff_timestamp_.store(0);

    /** 组装完成，返回数据 */
    if (!assembled.empty())
    {
#if RTP_DEBUG
        dlog_debug("RTP packet assembled: %d", nDebugCount);
#endif
        output = std::move(assembled);
        return true;
    }
    return false;
}

SIP::RTP::Packer::Packer()
{
    dlog_info("RTP Packer ssrc: %d", m_ssrc);
#if RTP_DEBUG_FILE
#if RTP_DEBUG_RTP_FILE
    m_rtpfile.open("./test.rtp", std::ios::out | std::ios::binary | std::ios::trunc);
#endif
#if RTP_DEBUG_PS_FILE
    m_psfile.open("./test.ps", std::ios::out | std::ios::binary | std::ios::trunc);
#endif
#if RTP_DEBUG_H264_FILE
    m_h264file.open("./test.h264", std::ios::out | std::ios::binary | std::ios::trunc);
#endif
#endif
    m_pPsEnc = std::make_shared<PS::PsEncoder>();
    m_pPsEnc->Init();
}

SIP::RTP::Packer::Packer(uint32_t ssrc, int fps, bool bIsTcp)
    : m_ssrc(ssrc),
      m_nVideoFps(fps),
      m_bIsTcp(bIsTcp),
      m_sn(0),
      m_timestamp(0),
      m_psIndex(0x40BF)
{
    dlog_info("RTP Packer ssrc: %d", m_ssrc);
#if RTP_DEBUG_FILE
#if RTP_DEBUG_RTP_FILE
    m_rtpfile.open("./test.rtp", std::ios::out | std::ios::binary | std::ios::trunc);
#endif
#if RTP_DEBUG_PS_FILE
    m_psfile.open("./test.ps", std::ios::out | std::ios::binary | std::ios::trunc);
#endif
#if RTP_DEBUG_H264_FILE
    m_h264file.open("./test.h264", std::ios::out | std::ios::binary | std::ios::trunc);
#endif
#endif
    m_pPsEnc = std::make_shared<PS::PsEncoder>();
    m_pPsEnc->Init();
}

SIP::RTP::Packer::~Packer()
{
#if RTP_DEBUG_FILE
#if RTP_DEBUG_RTP_FILE
    if (m_rtpfile.is_open())
    {
        m_rtpfile.close();
    }
#endif
#if RTP_DEBUG_H264_FILE
    if (m_h264file.is_open())
    {
        m_h264file.close();
    }
#endif
#if RTP_DEBUG_PS_FILE
    if (m_psfile.is_open())
    {
        m_psfile.close();
    }
#endif
#endif
    if (m_pPsEnc)
    {
        m_pPsEnc->Deinit();
        m_pPsEnc = nullptr;
    }
}

int RTP::Packer::packRtpPackage(
    char *pData, int nLen, bool bIsAudio,
    std::vector<std::vector<char>> &output)
{
#if RTP_DEBUG_FILE && RTP_DEBUG_H264_FILE
    if (m_h264file.is_open())
    {
        m_h264file.write(pData, nLen);
        m_h264file.flush();
    }
#endif
    /* NOTE 先封装PS格式再做RTP分包 */
    PS::PacketData_S packetInfo;
    packetInfo.pData = pData;
    packetInfo.nLen = nLen;
    packetInfo.bIsAudio = bIsAudio;
    packetInfo.nRtpTs = m_timestamp;
    packetInfo.nPsIndex = m_psIndex++; /* PS包的索引 */
    /* 暂定传入H264数据 */
    packetInfo.enVideo = PS::StreamType_E(m_nVideoType);
    packetInfo.enAudio = PS::StreamType_E(m_nAudioType);
    if (bIsAudio && packetInfo.enAudio == PS::StreamType_E::NONE)
    {
        dlog_error("PS流编码音频格式为空");
        return -1;
    }
    if ((!bIsAudio && packetInfo.enVideo == PS::StreamType_E::NONE))
    {
        dlog_error("PS流编码视频格式为空");
        return -2;
    }
    std::vector<char> outputPs;
    // PS::packet(&packetInfo, outputPs);
    if (m_pPsEnc)
    {
        int nRet = m_pPsEnc->EncData(&packetInfo, outputPs);
    }

    output.clear();
    size_t nOffSet = 0;
    size_t nTotal = outputPs.size();
    /* 每个RTP包实际最大可以携带的负载数据 */
    const size_t nMaxPayloadSize = RTP_MAX_MTU - RTP_HEADER_MIN_SIZE;
    /* NOTE 使用移动语义，不断获取头部数据，降低性能开销 */
    do
    {
        /* 分包最后一包的标记位 */
        bool bLastPacket = false;
        /* 计算这一次的实际负载数据大小 */
        size_t nActualSize = nTotal - nOffSet;
        if (nActualSize > nMaxPayloadSize)
        {
            /* 剩余的数据超过一包 */
            nActualSize = nMaxPayloadSize;
        }
        else
        {
            /* 剩余数据不足一包，更新最后一包的标记位 */
            bLastPacket = true;
        }

        /* 构建一个最小值的RTP数据头 */
        std::vector<char> rtpPacket;
        rtpPacket.resize(RTP_HEADER_MIN_SIZE, 0);
        /* 填充version,padding,extension,csrcCount */
        rtpPacket[0] |= 2 << 6;
        /* 填充marker(分包最后一包时设置为1) */
        rtpPacket[1] |= bLastPacket << 7;
        /* 填充payloadType */
        rtpPacket[1] |= m_payload & 0x7F;
        /* 填充sequenceNumber——末尾在递增sequenceNumber */
        rtpPacket[2] |= (m_sn >> 8) & 0xFF;
        rtpPacket[3] |= m_sn & 0xFF;
        /* 填充timestamp */
        rtpPacket[4] |= (m_timestamp >> 24) & 0xFF;
        rtpPacket[5] |= (m_timestamp >> 16) & 0xFF;
        rtpPacket[6] |= (m_timestamp >> 8) & 0xFF;
        rtpPacket[7] |= m_timestamp & 0xFF;
        /* 填充SSRC */
        rtpPacket[8] |= (m_ssrc >> 24) & 0xFF;
        rtpPacket[9] |= (m_ssrc >> 16) & 0xFF;
        rtpPacket[10] |= (m_ssrc >> 8) & 0xFF;
        rtpPacket[11] |= m_ssrc & 0xFF;
        if (m_bIsTcp)
        {
            int nRtpSize = nActualSize + RTP_HEADER_MIN_SIZE;
            /* RTP over TCP 需要插入2个字节表示RTP数据包的大小 */
            /* 先插入低8位，再插入高8位 */
            rtpPacket.insert(rtpPacket.begin(), (nRtpSize & 0xFF));
            rtpPacket.insert(rtpPacket.begin(), ((nRtpSize >> 8) & 0xFF));
        }
        /* 填充负载数据 */
        rtpPacket.insert(rtpPacket.end(),
                         outputPs.begin() + nOffSet,
                         outputPs.begin() + nOffSet + nActualSize);
#if RTP_DEBUG_FILE
        dlog_debug("SN[%05d]ActualSize[%d]OffSet[%ld]Total[%ld]FPS[%d]Timestamp[%ld]PacketSize[%ld]",
                   m_sn, nActualSize, nOffSet, nTotal, m_nVideoFps, m_timestamp, rtpPacket.size());
#endif
        /* 输入的输出的数组中 */
        output.push_back(std::move(rtpPacket));
        /* 更新数据 */
        nOffSet += nActualSize;
        m_sn++;
        m_sn = m_sn >= 0xFFFF ? 0 : m_sn;
    } while (nOffSet < nTotal);

    /* NOTE 音频时间戳沿用上一帧视频帧的时间戳 */
    /* 根据帧率，计算当前这一帧的RTP时间戳，确保从0开始 */
    if (!bIsAudio)
    {
        m_timestamp += RTPMAP_VIDEO_CLOCK / m_nVideoFps;
    }    

#if RTP_DEBUG_FILE && RTP_DEBUG_PS_FILE
    if (m_psfile.is_open())
    {
        m_psfile.write(outputPs.data(), outputPs.size());
        m_psfile.flush();
    }
#endif
#if RTP_DEBUG_FILE && RTP_DEBUG_RTP_FILE
    dlog_debug("组装了[%ld]个RTP包", output.size());
    if (m_rtpfile.is_open())
    {
        for (auto &pVec : output)
        {
            dlog_debug("RTP包大小[%ld]", pVec.size());
            m_rtpfile.write(pVec.data(), pVec.size());
            m_rtpfile.flush();
        }
    }
#endif
    return 0;
}
