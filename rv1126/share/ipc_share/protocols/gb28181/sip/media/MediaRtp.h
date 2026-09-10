/*
 * @Author       : EasonLu
 * @Date         : 2025-03-03 17:30:03
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-04-30 11:17:44
 * @FilePath     : MediaRtp.h
 * @Description  : RTP协议数据相关处理
 */
#pragma once
#include <atomic>
#include <cstdint>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#ifndef SIP_RTPMAP_DEFINE
#define SIP_RTPMAP_DEFINE
/* RTP协议支持的编码器 */
/* 参考GB/T+28181-2022 附录C */
/* 视频 */
#define RTPMAP_VIDEO_CLOCK 90000
#define RTPMAP_PS 96
#define RTPMAP_PS_CODEC "PS"

#define RTPMAP_MPEG4 97
#define RTPMAP_MPEG4_CODEC "MPEG4"

#define RTPMAP_H264 98
#define RTPMAP_H264_CODEC "H264"

#define RTPMAP_SVAC 99
#define RTPMAP_SVAC_CODEC "SVAC"

#define RTPMAP_H265 100
#define RTPMAP_H265_CODEC "H265"

/* 音频-通道数均为1，采样率均为8000Hz */
#define RTPMAP_PCMA 8
#define RTPMAP_PCMA_CODEC "PCMA"

#define RTPMAP_SVACA 20
#define RTPMAP_SVACA_CODEC "SVACA"

#define RTPMAP_G723 4
#define RTPMAP_G723_CODEC "G723"

#define RTPMAP_G729 18
#define RTPMAP_G729_CODEC "G729"

#define RTPMAP_G722 9
#define RTPMAP_G722_CODEC "G722"

#define RTPMAP_AAC 102
#define RTPMAP_AAC_CODEC "AAC"
#endif

/* 暂定MTU最大值 */
#ifndef RTP_MAX_MTU
#define RTP_MAX_MTU 1400
#endif

/* RTP头部最小长度 */
#ifndef RTP_HEADER_MIN_SIZE
#define RTP_HEADER_MIN_SIZE 12
#endif

namespace SIP
{
    namespace PS
    {
        class PsEncoder;
    };

    namespace RTP
    {

        /// @brief 回调解封出来的媒体数据
        typedef struct _MediaInfo_S_
        {
            uint64_t nTimestamp; /* 时间戳 */
            bool bIsAudio;       /* 是否音频 */
            const char *pData;   /* 数据指针 */
            size_t nLen;         /* 数据长度 */
            /* 从PSM中解析 */
            bool bIsKeyFrame;     /* 是否关键帧-视频帧获取到PSM则认定为当前帧为I帧 */
            uint32_t nStreamType; /* 流类型-区分音视频数据类型 */
        } MediaInfo_S;

        /// @brief 回调封装后的RTP数据包
        typedef struct _RtpInfo_S_
        {
            const char *pData; /* 数据指针 */
            size_t nLen;       /* 数据长度 */
        } RtpInfo_S;

        /**
         * @brief RTP 头部结构体
         */
        struct RtpHeader
        {
            uint8_t version;         /** RTP 版本号，占 2 位，通常为 2 */
            uint8_t padding;         /** 填充标志，占 1 位，若为 1，则数据末尾有额外填充字节 */
            uint8_t extension;       /** 扩展标志，占 1 位，若为 1，则 RTP 头部后有扩展数据 */
            uint8_t csrcCount;       /** CSRC 计数，占 4 位，指示 CSRC 标识符的数量 */
            uint8_t marker;          /** 标志位，占 1 位，指示是否为重要帧（如视频关键帧） */
            uint8_t payloadType;     /** 负载类型，占 7 位，表示传输的媒体格式（如 H264、AAC） */
            uint16_t sequenceNumber; /** 序列号，占 16 位，接收端可据此检测丢包，按 0-65535 递增 */
            uint32_t timestamp;      /** 时间戳，占 32 位，表示数据的采样时间，用于同步 */
            uint32_t ssrc;           /** 同步源标识符，占 32 位，区分不同流的来源 */
        };

        using RtpMediaDataCb = std::function<void(const MediaInfo_S &)>;

        /// @brief RTP的基类
        class Base
        {
        public:
            typedef std::shared_ptr<Base> Ptr;
            Base() = default;
            virtual ~Base() = default;
        };
        /**
         * @brief RTP 解析器类
         */
        class Parser : public Base
        {
        public:
            typedef std::shared_ptr<Parser> Ptr;
            /**
             * @brief 构造函数，指定SSRC
             * @param ssrc 指定要解析的SSRC
             */
            explicit Parser(uint32_t ssrc, RtpMediaDataCb fnCb);
            ~Parser();

            void reset(uint32_t ssrc);

            /**
             * @brief  解析RTP数据包
             * @param  [char] *data - 输入的 RTP 数据
             * @param  [size_t] size - 数据大小
             * @return [bool] 是否成功解析 RTP 负载
             * @author EasonLu
             * @note   UDP中发送的数据均为一个RTP包
             */
            bool parsePacket(const char *data, size_t size,bool bAudio);

            /**
             * @brief  解析RTP数据包
             * @param  [char] *data - 输入的 RTP 数据
             * @param  [size_t] size - 数据大小
             * @return [bool] 是否成功解析 RTP 负载
             * @author EasonLu
             * @note   TCP存在沾包情况，发送的数据有可能为多个RTP包
             */
            bool parsePacketTcp(const char *data, size_t size);

        private:
            /**
             * @brief 处理 RTP 分片数据
             * @param sequenceNumber RTP 序列号
             * @param timestamp 时间戳
             * @param marker 分片结束标志位
             * @param payload RTP 负载数据
             * @param output 组装完成的负载数据
             * @return 是否成功组装完整数据
             */
            bool processFragmentedPacket(uint16_t sequenceNumber, uint64_t timestamp, uint8_t marker, const std::vector<char> &payload, std::vector<char> &output);

            /**
             * @brief  重组缓存的RTP数据包
             * @param  [vector<char>] &output 输出重组后的数据
             * @return [*]
             * @author EasonLu
             * @note
             */
            bool makePacket(std::vector<char> &output);

            /* 调试写文件的句柄 */
            std::ofstream m_rtpfile;
            /* 调试写ps文件的句柄 */
            std::ofstream m_psfile;
            /** 指定的 SSRC */
            uint32_t m_ssrc;
            /* 解析数据回调 */
            RtpMediaDataCb m_fnMediaCb;
            /* 当前记录的分片缓存时间戳 */
            std::atomic<uint64_t> buff_timestamp_ = 0;
            /** RTP 分片缓存，仅存储当前 SSRC 的数据 */
            std::map<uint16_t, std::vector<char>> buffer_;
            /* 缓存TCP的数据,解决沾包问题 */
            std::vector<char> m_vecTcpBuffer;
        };

        class Packer : public Base
        {
        public:
            typedef std::shared_ptr<Packer> Ptr;
            Packer();
            Packer(uint32_t ssrc, int fps, bool bIsTcp = false);
            ~Packer();

            inline void setSSRC(uint32_t ssrc) { m_ssrc = ssrc; }
            inline void setVideoFps(int fps) { m_nVideoFps = fps; }
            inline void setVideoType(int nType) { m_nVideoType = nType; }
            inline void setAudioType(int nType) { m_nAudioType = nType; }
            inline void setPayloadType(uint8_t payload) { m_payload = payload; }

            inline void setIsTcp(bool tcp) { m_bIsTcp = tcp; }

            int packRtpPackage(char *pData, int nLen, bool bIsAudio, std::vector<std::vector<char>> &output);

        private:
            /* SSRC */
            uint32_t m_ssrc;
            /* 负载类型码（默认都是PS的负载码） */
            uint8_t m_payload = RTPMAP_PS;
            /* 视频帧率-自行计算PTS */
            int m_nVideoFps = 0;
            /* 视频格式-参照PS::StreamType_E的枚举值 */
            int m_nVideoType = 0;
            /* 音频格式-参照PS::StreamType_E的枚举值 */
            int m_nAudioType = 0;
            /* 是否为TCP */
            bool m_bIsTcp = false;
            /* 调试写文件的句柄 */
            std::ofstream m_rtpfile;
            /* 调试写ps文件的句柄 */
            std::ofstream m_psfile;
            /* 调试写文件的句柄 */
            std::ofstream m_h264file;
            /* SN */
            uint16_t m_sn = 0;
            /* 时间戳 */
            uint32_t m_timestamp = 0;
            /* PS包的索引 */
            uint32_t m_psIndex = 0;
            std::shared_ptr<PS::PsEncoder> m_pPsEnc = nullptr;
        };
    }
}