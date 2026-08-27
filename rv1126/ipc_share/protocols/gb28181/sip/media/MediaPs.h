/*
 * @Author       : EasonLu
 * @Date         : 2025-03-06 14:18:24
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-04-08 17:29:44
 * @FilePath     : MediaPs.h
 * @Description  : ps流的解封装
 */
#pragma once
#include "MediaRtp.h"
#include "mpeg-ps.h"
#include <stdint.h>
#include <vector>
#ifndef PS_HEADER
#define PS_HEADER (0xBA)
#endif

#ifndef PS_SYSTEM_HEADER
#define PS_SYSTEM_HEADER (0xBB)
#endif

#ifndef PS_PSM_HEADER
#define PS_PSM_HEADER (0xBC)
#endif

#ifndef PS_PSM_PRIVATE_HEADER
#define PS_PSM_PRIVATE_HEADER (0xBD)
#endif

#ifndef PS_PSM_PADDING_HEADER
#define PS_PSM_PADDING_HEADER (0xBE)
#endif

#ifndef PS_PSM_PRIVATE_HEADER_2
#define PS_PSM_PRIVATE_HEADER_2 (0xBF)
#endif

#ifndef PS_AUDIO_HEADER_START
#define PS_AUDIO_HEADER_START (0xC0)
#endif

#ifndef PS_AUDIO_HEADER_END
#define PS_AUDIO_HEADER_END (0xDF)
#endif

#ifndef PS_VIDEO_HEADER_START
#define PS_VIDEO_HEADER_START (0xE0)
#endif

#ifndef PS_VIDEO_HEADER_END
#define PS_VIDEO_HEADER_END (0xEF)
#endif

/* PES单包的负载数据上限 */
#ifndef PS_PES_MAX_PAYLOAD
#define PS_PES_MAX_PAYLOAD 65000
#endif

/* PS流系统时钟参考频率(Hz) */
#ifndef PS_SYSTEM_CLOCK
#define PS_SYSTEM_CLOCK 90000
#endif

/* PS流中复用速率(mux_rate)的每1MB/s的数据 */
#ifndef PS_MUX_RATE
#define PS_MUX_RATE 20972
#endif

/* PSHeader的大小固定值为20个字节 */
#ifndef PS_HEADER_FIX_SIZE
#define PS_HEADER_FIX_SIZE 20
#endif

namespace SIP
{
    namespace PS
    {
#pragma pack(push, 1) /* 设定结构体按 1 字节对齐，防止编译器自动填充对齐字节 */

        /**
         * @brief PS 头部（Program Stream Header）
         * PS 头部用于标识 PS 流的起始，并包含系统时钟参考（SCR）和复用速率信息
         * @note 固定结构，不可变
         */
        typedef struct
        {
            uint32_t start_code;          /** 固定值 0x000001BA，表示 PS 头的起始标志 */
            uint32_t marker1 : 2;         /** 固定值 01，表示标志位 */
            uint32_t scr_base1 : 3;       /** 系统时钟参考（SCR）的高 3 位 */
            uint32_t marker2 : 1;         /** 固定值 1，作为同步标志 */
            uint32_t scr_base2 : 15;      /** SCR 的中间 15 位 */
            uint32_t marker3 : 1;         /** 固定值 1，作为同步标志 */
            uint32_t scr_base3 : 15;      /** SCR 的低 15 位，共 33 位 SCR 用于同步音视频 */
            uint32_t marker4 : 1;         /** 固定值 1，作为同步标志 */
            uint32_t scr_ext : 9;         /** SCR 扩展位（9bit），用于提高精度 */
            uint32_t marker5 : 1;         /** 固定值 1，作为同步标志 */
            uint32_t mux_rate : 22;       /** 复用速率（单位：50 字节/秒） */
            uint32_t marker6 : 1;         /** 固定值 1，作为同步标志 */
            uint32_t marker7 : 1;         /** 固定值 1，作为同步标志 */
            uint32_t reserved : 5;        /** 保留位，固定值 0 */
            uint32_t stuffing_length : 3; /** PS 头填充字节的长度 */
        } PSHeader;

        /**
         * @brief PS 系统头（System Header）
         * 该结构体包含了整个 PS 码流的系统级信息
         * @note 固定结构，不可变
         */
        typedef struct
        {
            uint32_t start_code;                       /** 固定值 0x000001BB，表示系统头起始码 */
            uint32_t header_length : 16;               /** 系统头的总长度 */
            uint32_t marker_bit1 : 1;                  /** 固定值 1，作为同步标志 */
            uint32_t rate_bound : 22;                  /** 复用速率的低 22 位 */
            uint32_t marker_bit2 : 1;                  /** 固定值 1，作为同步标志 */
            uint32_t audio_bound : 6;                  /** 最大音频流数量 */
            uint32_t fixed_flag : 1;                   /** 是否固定码率 */
            uint32_t csps_flag : 1;                    /** 是否使用 CSPS */
            uint32_t system_audio_lock_flag : 1;       /** 音频锁定标志 */
            uint32_t system_video_lock_flag : 1;       /** 视频锁定标志 */
            uint32_t marker_bit3 : 1;                  /** 固定值 1，作为同步标志 */
            uint32_t video_bound : 5;                  /** 最大视频流数量 */
            uint32_t packet_rate_restriction_flag : 1; /** 包速率限制标志 */
            uint32_t reserved_bits : 7;                /** 保留位，固定值 0 */
            uint32_t stream_id : 8;                    /** 8位-区分流是音频还是视频 */
            uint32_t marker_bit4 : 2;                  /** 固定值 01，作为同步标志 */
            uint32_t p_std_buffer_bound_scale : 1;     /** 0以128字节为单位，1为1024为字节的单位 */
            uint32_t p_std_buffer_size_bound : 13;     /** 乘以p_std_buffer_bound_scale的总字节数缓冲区 */
        } PSSystemHeader;

        /// @brief PES头部的基础信息
        /// @note 扩展信息暂不处理
        /// @note 固定结构，不可变
        typedef struct _PESBase_S_
        {
            /* NOTE 24位-起始位的前三个字节，一般为0x000001 */
            uint32_t start_code : 24;
            uint32_t stream_id : 8;      /* 8位-区分流是音频还是视频 */
            uint32_t packet_length : 16; /* 16位-PES分组的字节数 */
            /* NOTE  packet_length 后面的字节高两位固定值'10' */
            uint32_t marker1 : 2;
            uint32_t scrambling_control : 2;        /* 2位-PES分组有效负载的加密模式，不加密时建议置0，否则置01 */
            uint32_t priority : 1;                  /* 1位-PES分组负载的优先级。1高优先级，0低优先级。非参考帧（包括B帧和，E帧，P帧）应置0，其余帧置1 */
            uint32_t data_alignment_indicator : 1;  /* 1位-置１表明此PES分组头部之后是data_stream_alignment_descriptor所定义的访问单元数据类型。若为‘０’，则没有定义是否对准。建议当是输入单元的第一个包时置1，其余置0。 */
            uint32_t copyright : 1;                 /* 1位-版权标志，置1表示版权，置0表示非版权 */
            uint32_t original_or_copy : 1;          /* 1位-置1表示原始数据，置0表示为转发数据 */
            uint32_t pts_dts_flag : 2;              /* 2位-标志PES首部含有PTS, DTS的状态。‘1x’表明PES首部含有PTS,若为‘x1’, 则首部含有DTS。 */
            uint32_t escr_flag : 1;                 /* 1位-置‘１’表示ESCR的base和extension字段都在PES分组首部出现；建议置0。 */
            uint32_t es_rate_flag : 1;              /* 1位-置‘１’表示PES首部有ES_rate字段；建议置0。 */
            uint32_t dsm_trick_mode_flag : 1;       /* 1位-置‘１’表示PES首部有 8bit的trick_mode_field；建议置0。 */
            uint32_t additional_copy_info_flag : 1; /* 1位-置‘１’表示有additional_copy_info字段；建议置0。 */
            uint32_t crc_flag : 1;                  /* 1位-置‘１’表示在PES首部分组中有CRC字段；建议置0。 */
            uint32_t extension_flag : 1;            /* 1位-置‘１’可选字段出现，当需要在每帧添加私有数据时建议置1，其余情况置0 */
            uint32_t header_data_length : 8;        /* 8位-表示PES分组首部中可选字段和填充字节的总字节数。 */
        } PESBase_S;

        /**
         * @brief PSM 头部（Program Stream Map Header）
         * PSM 用于描述该 PS 码流中的所有 PES（音视频流）
         */
        typedef struct
        {
            uint32_t prefix_start_code : 24;         /** PSM 头起始码，固定为 `0x000001BC` */
            uint32_t stream_id : 8;                  /** PSM ID */
            uint32_t psm_length : 16;                /** PSM 长度 */
            uint32_t curr_next_indicator : 1;        /** 当前/下一个指示符，0 表示当前，1 表示下一个 */
            uint32_t reserved1 : 2;                  /** 保留位 */
            uint32_t psm_version : 5;                /** PSM 版本号 */
            uint32_t reserver2 : 7;                  /** 保留位 */
            uint32_t marker_bit1 : 1;                /** 固定值 1，作为同步标志 */
            uint32_t ps_info_length : 16;            /** PS 信息长度 */
            uint32_t element_stream_map_length : 16; /** 元素流映射长度 */
            uint32_t crc_32;                         /** CRC-32 校验码 */
        } PSMHeader;

        /**
         * @brief PSM 流信息（Program Stream Map Stream Info）
         * 该结构体用于存储每个 PES 流的类型和 ID
         */
        typedef struct
        {
            uint32_t stream_type : 8;      /** PES 流的类型（视频、音频等） */
            uint32_t stream_id : 8;        /** PES 流 ID */
            uint32_t stream_info_len : 16; /** PES 流信息长度 */
            std::vector<char> stream_info; /** PES 流信息 */
        } PSMStreamInfo;

#pragma pack(pop) // 恢复默认结构体对齐

        /// @brief 头部类型
        typedef enum class _HeaderE_ : uint8_t
        {
            PS = 0,
            SYS,
            PSM,
            PES_VIDEO,
            PES_AUDIO,
            OTHER, /* 其他私有流暂不解析 */
            NONE,
        } Header_E;

        /// @brief 音视频流类型
        typedef enum _StreamTypeE_
        {
            NONE = 0x00,
            /* 视频相关 */
            MPEG_4 = 0x10, /* MPEG-4  */
            H264 = 0x1b,   /* H.264  */
            H265 = 0x24,   /* H.265 */
            SVAC_V = 0x80, /* SVAC视频 */
            /* 音频相关 */
            G711_A = 0x90, /* G711-A */
            G711_U = 0x91, /* G711-U */
            G722_1 = 0x92, /* G722.1 */
            G723_1 = 0x93, /* G723.1 */
            G729 = 0x99,   /* G729 */
            SVAC_A = 0x9b, /* SVAC音频 */
            AAC = 0x0f,    /* AAC */
        } StreamType_E;

        /**
         * @brief   解析PS数据
         * @param  [vector<char>] &input - 待解析的PS数据
         * @param  [RTP::RtpMediaDataCb] &fnCb - 媒体数据回调
         * @param  [uint64_t] nRtpTs - RTP时间戳
         * @return [bool] true为解析成功
         * @author EasonLu
         * @note
         */
        bool parse(const std::vector<char> &input, RTP::RtpMediaDataCb &fnCb, uint64_t nRtpTs = 0);

        /**
         * @brief  校验头部类型
         * @param  [char] chFourByte - 待校验的头部
         * @return [Header_E] 头部类型枚举值
         * @author EasonLu
         * @note
         */
        Header_E checkHeaderType(const char chFourByte);

        typedef struct _PacketData_S_
        {
            char *pData;
            int nLen;
            bool bIsAudio;
            uint64_t nRtpTs;
            uint32_t nPsIndex;
            StreamType_E enVideo;
            StreamType_E enAudio;
        } PacketData_S;

        int packet(PacketData_S *pPacket, std::vector<char> &output);

        class PsEncoder
        {
        public:
            PsEncoder();
            ~PsEncoder();

            int Init();
            int Deinit();
            int AddStream(int nStreamType);
            int EncData(PacketData_S *pPacket, std::vector<char> &vecOutput);

            /* PS回调函数 */
            static void *PsAllocCb(void *param, size_t bytes);
            static void PsFreeCb(void *param, void *packet);
            static int PsWriteCb(void *param, int stream, void *packet, size_t bytes);

        private:
            ps_muxer_t *m_psMuxer = nullptr;
            std::map<int, int> m_streamMap;
            std::vector<char> m_buffer;
        };
    }
}