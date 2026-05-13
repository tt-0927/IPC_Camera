#include "MediaPs.h"
#include "ModuleLog.h"
#include "SipUtils.h"
#include <ctime>
#include <map>
#include <string.h>
#include <sys/time.h>

#define MEDIA_PS_DEBUG 0
using namespace SIP;

/* H264的关键信息和关键帧*/
#define H264_IDR 5 /* IDR */
#define H264_SEI 6 /* SEI */
#define H264_SPS 7 /* SPS */
#define H264_PPS 8 /* PPS */
/* H265的关键信息和关键帧*/
#define H265_IDR_W_RADL 19 /* IDR */
#define H265_IDR_N_LP 20   /* IDR */
#define H265_VPS 32        /* VPS */
#define H265_SPS 33        /* SPS */
#define H265_PPS 34        /* PPS */
#define H265_SEI 39        /* SEI */

int check_iFrame(char *, int, bool);
int detect_nalu_type(char *, int, bool);

bool SIP::PS::parse(
    const std::vector<char> &input,
    SIP::RTP::RtpMediaDataCb &fnCb,
    uint64_t nRtpTs)
{
    uint64_t nOffset = 0;                            /* 整体偏移量 */
    uint64_t nTotal = input.size();                  /* 总长度 */
    std::map<uint32_t, PSMStreamInfo> mapStreamInfo; /* 媒体流信息 */

#if MEDIA_PS_DEBUG
    MLOG_DEBUG("PS解析数据大小[%llu]", nTotal);
#endif
    while (1)
    {
        if (nOffset >= nTotal)
        {
            break;
        }
        /* 根据前三位判断是否为PS数据 */
        if (input[nOffset + 0] != 0x00 && input[nOffset + 1] != 0x00 && input[nOffset + 2] != 0x01)
        {
            MLOG_WARN("不是PS数据 Offset[%llu] [%02x][%02x][%02x][%02x][%02x][%02x]",
                      nOffset,
                      input[nOffset + 0], input[nOffset + 1],
                      input[nOffset + 2], input[nOffset + 3],
                      input[nOffset + 4], input[nOffset + 5]);
            return false;
        }
        /* 根据第四位判断是哪种数据头 */
        auto enHeader = checkHeaderType(input[nOffset + 3]);
        if (enHeader == Header_E::NONE)
        {
            MLOG_WARN("无法解析的PS数据头[0x%02x]", input[nOffset + 3]);
            return false;
        }
#if MEDIA_PS_DEBUG
        MLOG_DEBUG("PS数据头类型[0x%02x] Offset[%llu] Total[%llu]", input[nOffset + 3], nOffset, nTotal);
#endif
        if (enHeader == Header_E::PS)
        {
            PSHeader header;
            memset(&header, 0, sizeof(PSHeader));
            header.start_code = (input[nOffset] << 24) | (input[nOffset + 1] << 16) | (input[nOffset + 2] << 8) | input[nOffset + 3];

            header.marker1 = (input[nOffset + 4] >> 6) & 0x03;
            header.scr_base1 = (input[nOffset + 4] >> 3) & 0x07;
            header.marker2 = (input[nOffset + 4] >> 2) & 0x01;
            header.scr_base2 = ((input[nOffset + 4] & 0x03) << 13) | (input[nOffset + 5] << 5) | (input[nOffset + 6] >> 3);
            header.marker3 = (input[nOffset + 6] >> 2) & 0x01;
            header.scr_base3 = ((input[nOffset + 6] & 0x03) << 13) | (input[nOffset + 7] << 5) | (input[nOffset + 8] >> 3);
            header.marker4 = (input[nOffset + 8] >> 2) & 0x01;
            header.scr_ext = ((input[nOffset + 8] & 0x03) << 7) | (input[nOffset + 9] >> 1);
            header.marker5 = input[nOffset + 9] & 0x01;
            header.mux_rate = (input[nOffset + 10] << 15) | (input[nOffset + 11] << 7) | (input[nOffset + 12] >> 2);
            header.marker6 = (input[nOffset + 12] >> 1) & 0x01;
            header.marker7 = input[nOffset + 12] & 0x01;
            header.reserved = (input[nOffset + 13] >> 3) & 0x1F;
            header.stuffing_length = input[nOffset + 13] & 0x07;
            nOffset += 14 + header.stuffing_length;
#if MEDIA_PS_DEBUG
            MLOG_DEBUG("PS数据头,SCR[%lld],MuxRate[%lld],StufLen[%d],Offset[%d]",
                       header.scr_base1 << 30 | header.scr_base2 << 15 | header.scr_base3,
                       header.mux_rate,
                       header.stuffing_length,
                       nOffset);
            for (int i = 0; i < header.stuffing_length; i++)
            {
                MLOG_DEBUG("PS数据头,Stuf[%d][0x%02x]", input[nOffset + 14 + i]);
            }
#endif
            continue;
        }

        if (enHeader == Header_E::SYS)
        {
            PSSystemHeader header;
            memset(&header, 0, sizeof(PSSystemHeader));
            header.start_code = (input[nOffset + 0] << 24) | (input[nOffset + 1] << 16) | (input[nOffset + 2] << 8) | input[nOffset + 3];
            /* NOTE header_length这个字段后面所有字段的大小（不包含额外数据） */
            header.header_length = (input[nOffset + 4] << 8) | input[nOffset + 5];

            header.marker_bit1 = (input[nOffset + 6] >> 7) & 0x01;
            header.rate_bound = ((input[nOffset + 6] & 0x7F) << 15) | (input[nOffset + 7] << 7) | (input[nOffset + 8] >> 1);
            header.marker_bit2 = input[nOffset + 8] & 0x01;
            header.audio_bound = (input[nOffset + 9] >> 2) & 0x3F;
            header.fixed_flag = (input[nOffset + 9] >> 1) & 0x01;
            header.csps_flag = input[nOffset + 9] & 0x01;
            header.system_audio_lock_flag = (input[nOffset + 10] >> 7) & 0x01;
            header.system_video_lock_flag = (input[nOffset + 10] >> 6) & 0x01;
            header.marker_bit3 = (input[nOffset + 10] >> 5) & 0x01;
            header.video_bound = (input[nOffset + 10] >> 0) & 0x1F;

            header.packet_rate_restriction_flag = (input[nOffset + 11] >> 7) & 0x01;
            header.reserved_bits = (input[nOffset + 11] >> 1) & 0x3F;

            /* 存在多组stream_id的信息，每3个字节为一组 */
            /* 防止有些SystemHeader没有添加stream_id信息导致后续读取错误，目前不读取stream_id的信息 */
#if 0
            header.stream_id = input[nOffset + 12];

            header.marker_bit4 = (input[nOffset + 13] >> 6) & 0x03;
            header.p_std_buffer_bound_scale = (input[nOffset + 13] >> 5) & 0x01;
            header.p_std_buffer_size_bound = ((input[nOffset + 13] & 0x1F) << 8) | input[nOffset + 14];
#endif
            /* NOTE 需要额外计算start_code的字节数 */
            nOffset += header.header_length + 4 + 2;
#if MEDIA_PS_DEBUG
            MLOG_DEBUG("SystemHeader数据头,header_length[%d],Rate[%d],AudioBound[%d],VideoBound[%d],Offset[%d]",
                       header.header_length,
                       header.rate_bound,
                       header.audio_bound,
                       header.video_bound,
                       nOffset);
#endif
            continue;
        }

        if (enHeader == Header_E::PSM)
        {
            PSMHeader header;
            memset(&header, 0, sizeof(PSMHeader));
            header.prefix_start_code = (input[nOffset + 0] << 16) | (input[nOffset + 1] << 8) | input[nOffset + 2];
            header.stream_id = input[nOffset + 3];
            /* NOTE psm_length这个字段后面所有字段的大小（不包含额外数据） */
            header.psm_length = (input[nOffset + 4] << 8) | input[nOffset + 5];

            header.curr_next_indicator = (input[nOffset + 6] >> 7) & 0x01;
            header.reserved1 = (input[nOffset + 6] >> 5) & 0x03;
            header.psm_version = input[nOffset + 6] & 0x1f;

            header.reserver2 = (input[nOffset + 7] >> 1) & 0x7f;
            header.marker_bit1 = input[nOffset + 7] & 0x01;

            header.ps_info_length = (input[nOffset + 8] << 8) | input[nOffset + 9];

            header.element_stream_map_length = (input[nOffset + 10 + header.ps_info_length] << 8) | input[nOffset + 11 + header.ps_info_length];
            uint32_t nStreamMapStart = 12 + header.ps_info_length;
            for (uint16_t nMapOffset = 0; nMapOffset < header.element_stream_map_length;)
            {
                PSMStreamInfo stStreamInfo;
                stStreamInfo.stream_type = input[nOffset + nStreamMapStart + nMapOffset];
                stStreamInfo.stream_id = input[nOffset + nStreamMapStart + nMapOffset + 1];
                stStreamInfo.stream_info_len |= (input[nOffset + nStreamMapStart + nMapOffset + 2] << 8);
                stStreamInfo.stream_info_len |= input[nOffset + nStreamMapStart + nMapOffset + 3];
                nMapOffset += 4;
                if (stStreamInfo.stream_info_len > 0)
                {
                    stStreamInfo.stream_info.insert(
                        stStreamInfo.stream_info.end(),
                        input.begin() + nOffset + nStreamMapStart + nMapOffset,
                        input.begin() + nOffset + nStreamMapStart + nMapOffset + stStreamInfo.stream_info_len);
                    nMapOffset += stStreamInfo.stream_info_len;
                }
                mapStreamInfo[stStreamInfo.stream_id] = stStreamInfo;
#if MEDIA_PS_DEBUG
                MLOG_DEBUG("PSMStreamInfo:Type[%02x],Id[%02x],InfoLen[%d]",
                           stStreamInfo.stream_type,
                           stStreamInfo.stream_id,
                           stStreamInfo.stream_info_len);
#endif
            }
            int nCrcStart = 12 + header.ps_info_length + header.element_stream_map_length;
            if (header.psm_length >= nCrcStart + 4)
            {
                /* 包含CRC_32校验码 */
                header.crc_32 |= (input[nOffset + nCrcStart + 1] << 24);
                header.crc_32 |= (input[nOffset + nCrcStart + 2] << 16);
                header.crc_32 |= (input[nOffset + nCrcStart + 3] << 8);
                header.crc_32 |= input[nOffset + nCrcStart + 4];
#if MEDIA_PS_DEBUG
                MLOG_DEBUG("PSM Header CRC_32[%08x]", header.crc_32);
#endif
            }
            int nPsmTotal = header.psm_length + 4 + 2;
            nOffset += nPsmTotal;

#if MEDIA_PS_DEBUG
            MLOG_DEBUG("PSM数据头,Total[%d],PsmLength[%d],CurrNextIndicator[%d],PsmVersion[%d],PsInfoLength[%d],ElementStreamMapLength[%d],Offset[%d]",
                       nPsmTotal,
                       header.psm_length,
                       header.curr_next_indicator,
                       header.psm_version,
                       header.ps_info_length,
                       header.element_stream_map_length,
                       nOffset);
#endif
            continue;
        }

        if (enHeader == Header_E::OTHER)
        {
            /* 其他私有流暂不解析，解析出大小后直接跳过 */
            uint64_t nOtherSize = input[nOffset + 4] << 8 | input[nOffset + 5];
            uint64_t nOtherTotal = nOtherSize + 4 + 2;
            nOffset += nOtherTotal;
#if MEDIA_PS_DEBUG
            MLOG_DEBUG("其他数据头,OtherSize[%llu],OtherTotal[%llu],Offset[%llu]",
                       nOtherSize,
                       nOtherTotal,
                       nOffset);
#endif
            continue;
        }

        if (enHeader == Header_E::PES_AUDIO || enHeader == Header_E::PES_VIDEO)
        {
            /* NOTE PES会存在分包的情况，视频分包必定连续 */
            std::vector<char> vecOutput;
            uint64_t nPts = 0;
            uint64_t nGetPts = 0;
            uint64_t nGetDts = 0;
            int nPesCount = 0;
            while (1)
            {
                /* 先解析PES包头数据再区分音视频数据 */
                PESBase_S header;
                memset(&header, 0, sizeof(PESBase_S));
                header.start_code = input[nOffset + 0] << 16 | input[nOffset + 1] << 8 | input[nOffset + 2];
                header.stream_id = input[nOffset + 3];
                /* NOTE packet_length这个字段后面所有字段的大小（不包含额外数据） */
                header.packet_length = input[nOffset + 4] << 8 | input[nOffset + 5];

                header.marker1 = input[nOffset + 6] >> 6 & 0x03;
                header.scrambling_control = (input[nOffset + 6] >> 4) & 0x03;
                header.priority = (input[nOffset + 6] >> 3) & 0x01;
                header.data_alignment_indicator = (input[nOffset + 6] >> 2) & 0x01;
                header.copyright = (input[nOffset + 6] >> 1) & 0x01;
                header.original_or_copy = input[nOffset + 6] & 0x01;

                header.pts_dts_flag = (input[nOffset + 7] >> 6) & 0x03;
                header.escr_flag = (input[nOffset + 7] >> 5) & 0x01;
                header.es_rate_flag = (input[nOffset + 7] >> 4) & 0x01;
                header.dsm_trick_mode_flag = (input[nOffset + 7] >> 3) & 0x01;
                header.additional_copy_info_flag = (input[nOffset + 7] >> 2) & 0x01;
                header.crc_flag = (input[nOffset + 7] >> 1) & 0x01;
                header.extension_flag = input[nOffset + 7] & 0x01;

                header.header_data_length = input[nOffset + 8];

                if (header.pts_dts_flag & 0x02)
                {
                    uint16_t pts1 = (input[nOffset + 9] >> 1) & 0x07;
                    uint16_t pts2 = (input[nOffset + 10] << 7) | (input[nOffset + 11] >> 1);
                    uint16_t pts3 = (input[nOffset + 12] << 7) | (input[nOffset + 13] >> 1);
                    nGetPts = pts1 << 30 | pts2 << 15 | pts3;
#if MEDIA_PS_DEBUG
                    MLOG_DEBUG("PES PTS[0x%x][0x%x][0x%x][%llu]", pts1, pts2, pts3, nGetPts);
#endif
                }

                if (header.pts_dts_flag & 0x01)
                {
                    uint16_t dts1 = (input[nOffset + 14] >> 1) & 0x07;
                    uint16_t dts2 = (input[nOffset + 15] << 7) | (input[nOffset + 16] >> 1);
                    uint16_t dts3 = (input[nOffset + 17] << 7) | (input[nOffset + 18] >> 1);
                    nGetDts = dts1 << 30 | dts2 << 15 | dts3;
#if MEDIA_PS_DEBUG
                    MLOG_DEBUG("PES DTS[0x%x][0x%x][0x%x][%llu]", dts1, dts2, dts3, nGetDts);
#endif
                }
                /* NOTE PES数据头的总字节数，需要额外加上start_code，stream_id，packet_length的字节数 */
                uint32_t nPesTotal = header.packet_length + 3 + 1 + 2;
                uint32_t nPesHeaderTotal = 9 + header.header_data_length;
                uint32_t nDataStart = nOffset + nPesHeaderTotal;
                uint32_t nDataSize = nOffset + nPesTotal - nDataStart;
#if MEDIA_PS_DEBUG
                MLOG_DEBUG("PES数据[%02x],pts_flag[%d],packet[%d],header[%d],offset[%d],start[%d][%d]nPesTotal[%d]nPesHeaderTotal[%d]",
                           header.stream_id,
                           header.pts_dts_flag,
                           header.packet_length,
                           header.header_data_length,
                           nOffset,
                           nDataStart,
                           nDataSize,
                           nPesTotal,
                           nPesHeaderTotal);
#endif
                /* 记录pts */
                if (nGetPts > 0)
                {
                    /* 有PTS则首先使用PTS的记录 */
                    nPts = nGetPts;
                }
                else
                {
                    /* PTS没记录则使用DTS，DTS没记录则使用上一包的PTS */
                    nPts = nGetDts > 0 ? nGetDts : nPts;
                }
                /* 记录分包数 */
                nPesCount++;
                /* 存储PES数据 */
                vecOutput.insert(vecOutput.end(), input.begin() + nDataStart, input.begin() + nDataStart + nDataSize);
                /* 偏移PES数据 */
                nOffset += nPesTotal;
                if (nOffset >= nTotal)
                {
                    break; /* 退出分包循环 */
                }
                auto enNextHeader = checkHeaderType(input[nOffset + 3]);

                /* 退出条件 */
                if ((enHeader == Header_E::PES_VIDEO && enNextHeader != enHeader) ||
                    enHeader == Header_E::PES_AUDIO)
                {
                    /* NOTE
                        1、首包为视频包，连续的包不为视频包时，认定PES的视频分包结束
                        2、音频包达不到分包上限的0xFFFF
                     */
                    break;
                }
            }
            /* 提取音视频数据进行上抛 */
            if (fnCb)
            {
#if MEDIA_PS_DEBUG
                MLOG_DEBUG("PES数据上抛组包数[%d] [0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x]",
                           nPesCount,
                           vecOutput[0], vecOutput[1],
                           vecOutput[2], vecOutput[3],
                           vecOutput[4], vecOutput[5]);
#endif
                SIP::RTP::MediaInfo_S stCbData;
                /* 优先使用RTP数据头的时间戳 */
                stCbData.nTimestamp = nRtpTs > 0 ? nRtpTs : nPts;
                stCbData.pData = vecOutput.data();
                stCbData.nLen = vecOutput.size();
                stCbData.bIsAudio = enHeader == Header_E::PES_AUDIO ? true : false;
                stCbData.bIsKeyFrame = false; /* 默认填false */
                stCbData.nStreamType = 0;     /* 默认填0 */
                if (mapStreamInfo.size() > 0)
                {
                    stCbData.bIsKeyFrame = enHeader == Header_E::PES_VIDEO;
                    auto pFindKey = stCbData.bIsKeyFrame ? PS_VIDEO_HEADER_START : PS_AUDIO_HEADER_START;
                    auto pFind = mapStreamInfo.find(pFindKey);
                    if (pFind != mapStreamInfo.end())
                    {
                        stCbData.nStreamType = pFind->second.stream_type;
                    }
                }

                fnCb(stCbData);
            }
        }
    }
    /* 解析不成功则会提前返回false */
    return true;
}

SIP::PS::Header_E SIP::PS::checkHeaderType(const char chFourByte)
{
    Header_E enHeader = Header_E::NONE;
    if (chFourByte == PS_HEADER)
    {
        enHeader = Header_E::PS;
    }
    else if (chFourByte == PS_SYSTEM_HEADER)
    {
        enHeader = Header_E::SYS;
    }
    else if (chFourByte == PS_PSM_HEADER)
    {
        enHeader = Header_E::PSM;
    }
    else if (chFourByte >= PS_AUDIO_HEADER_START && chFourByte <= PS_AUDIO_HEADER_END)
    {
        enHeader = Header_E::PES_AUDIO;
    }
    else if (chFourByte >= PS_VIDEO_HEADER_START && chFourByte <= PS_VIDEO_HEADER_END)
    {
        enHeader = Header_E::PES_VIDEO;
    }
    else if (chFourByte == PS_PSM_PRIVATE_HEADER ||
             chFourByte == PS_PSM_PADDING_HEADER ||
             chFourByte == PS_PSM_PRIVATE_HEADER_2)
    {
        enHeader = Header_E::OTHER;
    }
    /* 其他类型的数据头不解析 */
    return enHeader;
}

int detect_nalu_type(char *data, int dataSize, bool bIsH265)
{
    // 检查数据指针是否为空
    if (data == NULL || dataSize <= 0)
    {
        return -1;
    }
    int nNaluType = -1;
    unsigned char checkByte = 0;
    /* 处理4字节起始码(0x00000001) */
    if (dataSize >= 5 &&
        data[0] == 0x00 &&
        data[1] == 0x00 &&
        data[2] == 0x00 &&
        data[3] == 0x01)
    {
        checkByte = data[4];
    }
    /* 处理3字节起始码(0x000001) */
    else if (dataSize >= 4 &&
             data[0] == 0x00 &&
             data[1] == 0x00 &&
             data[2] == 0x01)
    {
        checkByte = data[3];
    }
    else
    {
        return -2;
    }

    if (checkByte & 0x80)
    {
        /* 禁止位不能为1 */
        return -3;
    }

    if (bIsH265)
    {
        /* H265检测中间6位 */
        nNaluType = (checkByte >> 1) & 0x3f;
    }
    else
    {
        /* H264检测低5位 */
        nNaluType = checkByte & 0x1f;
    }
#if MEDIA_PS_DEBUG
    MLOG_DEBUG("detect_nalu_type[%d]IsH265[%d]=>[%02x][%02x][%02x][%02x][%02x][%02x]",
               nNaluType, bIsH265, data[0], data[1], data[2], data[3], data[4], data[5]);
#endif
    return nNaluType;
}

int check_iFrame(char *pchData, int dataSize, bool bIsH265)
{
    int nNaluType = detect_nalu_type(pchData, dataSize, bIsH265);
#if MEDIA_PS_DEBUG
    if (dataSize >= 6)
    {
        // MLOG_DEBUG("check_iFrame[%02x][%02x][%02x][%02x][%02x][%02x]NALU[%d]",
        //            pchData[0], pchData[1], pchData[2],
        //            pchData[3], pchData[4], pchData[5],
        //            nNaluType);
    }
#endif
    if (nNaluType < 0)
    {
        return -1;
    }
    if (bIsH265)
    {
        switch (nNaluType)
        {
        case H265_IDR_N_LP:
        case H265_IDR_W_RADL:
        case H265_VPS:
        case H265_SPS:
        case H265_PPS:
        case H265_SEI:
            return nNaluType;
        default:
            break;
        }
    }
    else
    {
        switch (nNaluType)
        {
        case H264_IDR:
        case H264_SEI:
        case H264_SPS:
        case H264_PPS:
            return nNaluType;
        default:
            break;
        }
    }

    /* 其他的NALU类型不是关键帧或关键信息 */
    return 0;
}

int SIP::PS::packet(
    PacketData_S *pPacket,
    std::vector<char> &output)
{
    if (pPacket == nullptr)
    {
        return -1;
    }
    bool bIsH265 = pPacket->enVideo == StreamType_E::H265;
    bool bHaveAudio = pPacket->enAudio != StreamType_E::NONE;
    int nIRet = check_iFrame(pPacket->pData, pPacket->nLen, bIsH265);
    bool bIsIFrame = nIRet > 0; /* 大于0为关键信息或I帧 */
#ifdef MEDIA_PS_DEBUG
    MLOG_DEBUG("当前数据是否为I帧[%d]音频帧[%d]nIRet[%d]",
               bIsIFrame, pPacket->bIsAudio, nIRet);
#endif
    /* 暂定填写8M码率 */
    uint32_t nMuxRate = PS_MUX_RATE * 8;

    /* 获取当前时间（包括毫秒） */
    struct timeval tv;
    gettimeofday(&tv, nullptr);

    /* 转换为本地时间 */
    time_t t = tv.tv_sec;
    std::tm *ptm = std::localtime(&t);

    /* 获取毫秒部分 */
    int millis = tv.tv_usec / 1000 % 1000;

    /* NOTE 把时分秒统一换算为总毫秒数 */
    uint64_t nTotalSec = (ptm->tm_hour * 3600 + ptm->tm_min * 60 + ptm->tm_sec) * 1000 + millis;
    /* 直接转换scr_base值，因为转换为毫秒数，额外除以1000 */
    nTotalSec = nTotalSec * PS_SYSTEM_CLOCK / 1000;
    /* NOTE 不使用scr_extension，时钟精度足够 */
    /* 清空数据 */
    output.clear();
    { /* 构建PS数据头 */
        /* 适配不同的大小端，直接赋值 */
        std::vector<char> vecHeader;
        /* 设置PS数据头的固定大小 */
        vecHeader.resize(PS_HEADER_FIX_SIZE);
        /* start_code */
        vecHeader[0] = 0x00;
        vecHeader[1] = 0x00;
        vecHeader[2] = 0x01;
        vecHeader[3] = PS_HEADER;
        /* 固定高2位'01' */
        vecHeader[4] |= 1 << 6;
        vecHeader[4] |= (nTotalSec >> 30 & 0x7) << 3;
        vecHeader[4] |= 1 << 2;
        vecHeader[4] |= (nTotalSec >> 28 & 0x3);

        vecHeader[5] |= (nTotalSec >> 20 & 0xFF);

        vecHeader[6] |= (nTotalSec >> 15 & 0x1F) << 3;
        vecHeader[6] |= 1 << 2;
        vecHeader[6] |= (nTotalSec >> 13 & 0x3);

        vecHeader[7] |= (nTotalSec >> 5 & 0xFF);

        vecHeader[8] |= (nTotalSec & 0x1F) << 3;
        vecHeader[8] |= 1 << 2;
        vecHeader[8] &= 0xFC; /* scr_extension的9位数据的高2位置0 */

        vecHeader[9] &= 0x01; /* scr_extension的9位数据的低7位置0 */
        vecHeader[9] |= 1;

        vecHeader[10] |= (nMuxRate >> 14 & 0xFF);

        vecHeader[11] |= (nMuxRate >> 6 & 0xFF);

        vecHeader[12] |= (nMuxRate & 0x3F) << 2;
        vecHeader[12] |= 2;

        vecHeader[13] = 0xF8;       /* 前五位reserved置0 */
        vecHeader[13] |= (6 & 0x7); /* 默认填充6个字节 */
        /* 填充字节数，确保PS头有20个字节 */

        vecHeader[14] = 0xFF;
        vecHeader[15] = 0xFF;
#if 1
        vecHeader[16] = (pPacket->nPsIndex >> 24) & 0xFF;
        vecHeader[17] = (pPacket->nPsIndex >> 16) & 0xFF;
        vecHeader[18] = (pPacket->nPsIndex >> 8) & 0xFF;
        vecHeader[19] = pPacket->nPsIndex & 0xFF;
#else
        vecHeader[16] = 0xFF;
        vecHeader[17] = 0xFF;
        vecHeader[18] = 0xFF;
        vecHeader[19] = 0xFF;
#endif
        output.insert(output.end(), vecHeader.begin(), vecHeader.end());
#ifdef MEDIA_PS_DEBUG
        char *pHeader = vecHeader.data();
        MLOG_DEBUG("PSHeader[%d][%02x][%02x][%02x][%02x][%02x][%02x][%02x][%02x]",
                   vecHeader.size(),
                   pHeader[0], pHeader[1], pHeader[2], pHeader[3],
                   pHeader[4], pHeader[5], pHeader[6], pHeader[7]);
#endif
    }

    /* 暂定只给I帧添加SystemHeader和PSMHeader */
    if (bIsIFrame && !pPacket->bIsAudio)
    {
        { /* 构建SystemHeader */
            /* 64x1024的缓冲区 */
            int buffer_size_bound = 64;
            std::vector<char> vecHeader; /* 插入数据再做写入 */
            /* start_code */
            vecHeader.push_back(0);
            vecHeader[0] = 0x00;
            vecHeader.push_back(0);
            vecHeader[1] = 0x00;
            vecHeader.push_back(0);
            vecHeader[2] = 0x01;
            vecHeader.push_back(0);
            vecHeader[3] = PS_SYSTEM_HEADER;
            /* header_length,后续再回填 */
            vecHeader.push_back(0);
            vecHeader[4] |= 0x00;
            vecHeader.push_back(0);
            vecHeader[5] |= 0x00;
            /* marker_bit1 */
            vecHeader.push_back(0);
            vecHeader[6] |= 1 << 7;
            /* rate_bound */
            vecHeader[6] |= (nMuxRate >> 15 & 0x7F);
            /* rate_bound */
            vecHeader.push_back(0);
            vecHeader[7] |= (nMuxRate >> 7 & 0xFF);
            /* rate_bound */
            vecHeader.push_back(0);
            vecHeader[8] |= (nMuxRate & 0x7F) << 1;
            /* marker_bit2 */
            vecHeader[8] |= 1;
            /* audio_bound,fixed_flag,csps_flag */
            vecHeader.push_back(0);
            vecHeader[9] |= bHaveAudio ? 1 : 0 << 2;
            /* system_audio_lock_flag,system_video_lock_flag,marker_bit3,video_bound */
            vecHeader.push_back(0);
            vecHeader[10] = 0xE1;
            /* packet_rate_restriction_flag,reserved_bits */
            vecHeader.push_back(0);
            vecHeader[11] = 0x00;
            /* 添加视频组stream_id的数据 */
            vecHeader.push_back(0);
            vecHeader[12] = PS_VIDEO_HEADER_START;
            vecHeader.push_back(0);
            vecHeader[13] |= 2 << 6; /* 固定占位'11' */
            vecHeader[13] |= 1 << 5; /* p_std_buffer_bound_scale */
            vecHeader[13] |= (buffer_size_bound >> 8 & 0x1F);
            vecHeader.push_back(0);
            vecHeader[14] |= buffer_size_bound & 0xFF;
            /* 添加音频组stream_id的数据 */
            vecHeader.push_back(0);
            vecHeader[15] = PS_AUDIO_HEADER_START;
            vecHeader.push_back(0);
            vecHeader[16] |= 2 << 6; /* 固定占位'11' */
            vecHeader[16] |= 1 << 5; /* p_std_buffer_bound_scale */
            vecHeader[16] |= (buffer_size_bound >> 8 & 0x1F);
            vecHeader.push_back(0);
            vecHeader[17] |= buffer_size_bound & 0xFF;

            /* 回填数据头的长度，扣掉start_code和header_length */
            int nHeaderLen = vecHeader.size() - 4 - 2;
            vecHeader[4] |= (nHeaderLen >> 8 & 0xFF);
            vecHeader[5] |= (nHeaderLen & 0xFF);

            output.insert(output.end(), vecHeader.begin(), vecHeader.end());
#ifdef MEDIA_PS_DEBUG
            char *pHeader = vecHeader.data();
            MLOG_DEBUG("PSSystemHeader[%d][%02x][%02x][%02x][%02x][%02x][%02x][%02x][%02x]",
                       vecHeader.size(),
                       pHeader[0], pHeader[1], pHeader[2], pHeader[3],
                       pHeader[4], pHeader[5], pHeader[6], pHeader[7]);
#endif
        }

        { /* 构建PSMHeader */
            /* 插入数据再做写入 */
            std::vector<char> vecHeader;
            /* start_code */
            vecHeader.push_back(0);
            vecHeader[0] = 0x00;
            vecHeader.push_back(0);
            vecHeader[1] = 0x00;
            vecHeader.push_back(0);
            vecHeader[2] = 0x01;
            vecHeader.push_back(0);
            vecHeader[3] = PS_PSM_HEADER;
            /* psm_length,具体长度后续回填 */
            vecHeader.push_back(0);
            vecHeader[4] |= 0x00;
            vecHeader.push_back(0);
            vecHeader[5] |= 0x00;

            /* curr_next_indicator,reserved1,psm_version */
            vecHeader.push_back(0);
            vecHeader[6] |= 1 << 7;
            vecHeader[6] |= 2 << 5;   /* reserved1置为'11' */
            vecHeader[6] |= 1 & 0x1F; /* 默认psm_version为1 */

            /* reserver2,marker_bit1 */
            vecHeader.push_back(0);
            vecHeader[7] |= 0xFF;

            /* ps_info_length */
            vecHeader.push_back(0); /* 8 */
            vecHeader.push_back(0); /* 9 */

            /* element_stream_map_length，具体长度后续回填 */
            vecHeader.push_back(0); /* 10 */
            vecHeader.push_back(0); /* 11 */

            /* 构建节目映射表 */
            int nStreamMapLen = 0;
            if (pPacket->enVideo > StreamType_E::NONE)
            {
                vecHeader.push_back(pPacket->enVideo);
                vecHeader.push_back(PS_VIDEO_HEADER_START);
                /* 插入两位，表示stream_info_len的长度为0 */
                vecHeader.push_back(0);
                vecHeader.push_back(0);
                nStreamMapLen += 4;
            }

            if (pPacket->enAudio > StreamType_E::NONE)
            {
                vecHeader.push_back(pPacket->enAudio);
                vecHeader.push_back(PS_AUDIO_HEADER_START);
                /* 插入两位，表示stream_info_len的长度为0 */
                vecHeader.push_back(0);
                vecHeader.push_back(0);
                nStreamMapLen += 4;
            }
            /* 回填element_stream_map_length */
            vecHeader[10] |= (nStreamMapLen >> 8 & 0xFF);
            vecHeader[11] |= (nStreamMapLen & 0xFF);

            { /* crc_32 */
                /* 计算CRC32要从psm_length开始，需要跳过前面的字段，并且重新计算大小 */
                uint32_t nCrc32 = CalcCRC_32(vecHeader.data() + 4 + 2,
                                             vecHeader.size() - 4 - 2);
                vecHeader.push_back(nCrc32 & 0xFF);
                vecHeader.push_back((nCrc32 >> 8) & 0xFF);
                vecHeader.push_back((nCrc32 >> 16) & 0xFF);
                vecHeader.push_back((nCrc32 >> 24) & 0xFF);
            }

            /* 计算总大小，扣掉start_code和psm_length */
            int nPsmLen = vecHeader.size() - 4 - 2;
            vecHeader[4] |= (nPsmLen >> 8 & 0xFF);
            vecHeader[5] |= (nPsmLen & 0xFF);

            output.insert(output.end(), vecHeader.begin(), vecHeader.end());
#ifdef MEDIA_PS_DEBUG
            char *pHeader = vecHeader.data();
            MLOG_DEBUG("PSMHeader[%d][%02x][%02x][%02x][%02x][%02x][%02x][%02x][%02x]",
                       vecHeader.size(),
                       pHeader[0], pHeader[1], pHeader[2], pHeader[3],
                       pHeader[4], pHeader[5], pHeader[6], pHeader[7]);
#endif
        }
    }

    {
        /* 构建PES包 */
        int nOffset = 0;
        bool bFindIFrame = false;
        /* 数据超过2^16 - 1 时需要进行分包封装 */
        do
        {
            /* 记录整个包的长度 */
            size_t nPesHeaderSize = 0;
            std::vector<char> vecHeader;
            /* start_code */
            vecHeader.push_back(0);
            vecHeader[0] = 0x00;
            vecHeader.push_back(0);
            vecHeader[1] = 0x00;
            vecHeader.push_back(0);
            vecHeader[2] = 0x01;
            vecHeader.push_back(0);
            vecHeader[3] = pPacket->bIsAudio ? PS_AUDIO_HEADER_START : PS_VIDEO_HEADER_START;
            /* packet_length,后续再回填,包含负载数据的大小，不包括start_code和packet_length */
            vecHeader.push_back(0);
            vecHeader[4] |= 0x00;
            vecHeader.push_back(0);
            vecHeader[5] |= 0x00;

            vecHeader.push_back(0);
            /* marker1 */
            vecHeader[6] |= 2 << 6;
            /* scrambling_control */
            vecHeader[6] |= (0 & 0x3) << 4;
            /* priority */
            vecHeader[6] |= (bIsIFrame ? 1 : 0) << 3;
            /* data_alignment_indicator */
            vecHeader[6] |= (nOffset == 0) << 2;
            /* copyright,original_or_copy */
            vecHeader[6] |= 1;

            vecHeader.push_back(0);
            /* pts_dts_flag,只填PTS，不填DTS */
            vecHeader[7] |= (nOffset == 0 ? 2 : 0) << 6;

            int nHeaderDataLen = 2;
            /* header_data_length，后续回填，最少填充2个字节，需要包括PTS的字节数 */
            vecHeader.push_back(0); /* 8 */

            /* 使用传入的RtpTs作为Pts */
            if (nOffset == 0)
            {
                std::vector<char> vecPts;
                vecPts.resize(5, '0'); /* pts占5字节 */
                /* | 32-30 (3位) | 29-22 (8位) | 21-15 (7位) | 14-7 (8位) | 6-0 (7位) | */
                /* pts固定高4位'0010' */
                vecPts[0] |= 0x2 << 4;
                /* pts的第32~30位 */
                vecPts[0] |= ((pPacket->nRtpTs >> 30) & 0x7) << 1;
                /* 间隔标记位，置1 */
                vecPts[0] |= 1;
                /* pts的第29~15位 */
                vecPts[1] |= ((pPacket->nRtpTs >> 22) & 0xff);
                vecPts[2] |= ((pPacket->nRtpTs >> 15) & 0x7f) << 1;
                vecPts[2] |= 1;
                /* pts的第14~0位 */
                vecPts[3] |= ((pPacket->nRtpTs >> 7) & 0xff);
                vecPts[4] |= ((pPacket->nRtpTs & 0x7f)) << 1;
                vecPts[4] |= 1;
                /* 记录添加了pts */
                nHeaderDataLen += vecPts.size();
                /* 直接追加 */
                vecHeader.insert(vecHeader.end(), vecPts.begin(), vecPts.end());
            }

            /* 计算负载数据值 */
            int nPayload = 0;
            int nIDROffset = 0;
            /* 这一次的数据是包含SPS、PPS、SEI等关键信息和I帧数据 */
            if (bIsIFrame && !bFindIFrame)
            {
                /* 默认偏移多几个字节，否则会连续判断失误 */
                nIDROffset = nOffset + 4;
                while (1)
                {
                    int nNaluType = detect_nalu_type(pPacket->pData + nIDROffset,
                                                     pPacket->nLen - nIDROffset,
                                                     bIsH265);
                    if (nNaluType > 0)
                    {
                        /* 找到NALU，需要判断是否为I帧 */
                        if (bIsH265)
                        {
                            if (nNaluType == H265_IDR_N_LP || nNaluType == H265_IDR_W_RADL)
                            {
                                bFindIFrame = true;
                            }
                        }
                        else
                        {
                            if (nNaluType == H264_IDR)
                            {
                                bFindIFrame = true;
                            }
                        }

                        if (bFindIFrame)
                        {
#ifdef MEDIA_PS_DEBUG
                            MLOG_DEBUG("[PES]找到I帧信息");
#endif
                            bFindIFrame = true;
                        }
                        break;
                    }
                    /* 找不到则继续往后偏移 */
                    ++nIDROffset;
                    if (nIDROffset >= pPacket->nLen)
                    {
                        /* 超过数据范围也退出 */
                        break;
                    }
                }
            }
#ifdef MEDIA_PS_DEBUG
            MLOG_DEBUG("[PES]nIDROffset[%d]nOffset[%d]len[%d]",
                       nIDROffset, nOffset, pPacket->nLen);
#endif
            nIDROffset = nIDROffset == 0 ? pPacket->nLen : nIDROffset;
            nPayload = nIDROffset - nOffset;

            if (nPayload > PS_PES_MAX_PAYLOAD)
            {
                /* 剩余数据超过一包 */
                nPayload = PS_PES_MAX_PAYLOAD;
            }

            nPesHeaderSize = nPayload + vecHeader.size();
            /* 计算整个 (PES包 + 可选字段和填充字节的总数) 是否4字节对齐 */
            int nMod = (nPesHeaderSize) % 4;
            if (nMod > 0)
            {
                /* 需要补齐 */
                nHeaderDataLen += nMod;
                nPesHeaderSize += nMod;
                for (int i = 0; i < nMod; i++)
                {
                    vecHeader.push_back(0xFF);
                }
            }
            /* 回填扩展和填充的数据 */
            vecHeader[8] |= nHeaderDataLen & 0xFF;

            /* 回填填充后的总长度，扣点start_code和packet_length */
            nPesHeaderSize = nPesHeaderSize - 4 - 2;
            vecHeader[4] |= (nPesHeaderSize >> 8 & 0xFF);
            vecHeader[5] |= (nPesHeaderSize & 0xFF);

            /* 写入包头数据 */
            output.insert(output.end(), vecHeader.begin(), vecHeader.end());

            /* 填充负载数据 */
            output.insert(output.end(),
                          pPacket->pData + nOffset,
                          pPacket->pData + nOffset + nPayload);
            /* 更新偏移值 */
            nOffset += nPayload;
#ifdef MEDIA_PS_DEBUG
            MLOG_DEBUG("PES纯包头长度[%ld]Payload[%d]packet_len[%ld]Mod[%d]",
                       vecHeader.size(),
                       nPayload,
                       nPesHeaderSize,
                       nMod);
#endif
        } while (nOffset < pPacket->nLen);
    }
    return 0;
}

SIP::PS::PsEncoder::PsEncoder()
    : m_psMuxer(nullptr)
{
    m_streamMap.clear();
}

SIP::PS::PsEncoder::~PsEncoder()
{
    Deinit();
}

int SIP::PS::PsEncoder::Init()
{
    struct ps_muxer_func_t handler;
    handler.alloc = PsEncoder::PsAllocCb;
    handler.free = PsEncoder::PsFreeCb;
    handler.write = PsEncoder::PsWriteCb;
    m_psMuxer = ps_muxer_create(&handler, (void *)this);
    if (nullptr == m_psMuxer)
    {
        return -1;
    }
    return 0;
}

int SIP::PS::PsEncoder::Deinit()
{
    if (m_psMuxer)
    {
        ps_muxer_destroy(m_psMuxer);
        m_psMuxer = nullptr;
    }
    return 0;
}

int SIP::PS::PsEncoder::AddStream(int nStreamType)
{
    if (m_psMuxer)
    {
        auto nStreamID = ps_muxer_add_stream(m_psMuxer, nStreamType, NULL, 0);
        if (nStreamID < 0)
        {
            return -1;
        }
        m_streamMap[nStreamType] = nStreamID;
    }
    return 0;
}

int SIP::PS::PsEncoder::EncData(PacketData_S *pPacket, std::vector<char> &vecOutput)
{
    if (nullptr == pPacket)
    {
        return -1;
    }

    if (nullptr == m_psMuxer)
    {
        return -2;
    }
    /* NOTE 封装过程为同步进行的，只需等待封装完毕后即可返回数据 */
    size_t nRet = 0;
    int nStreamType = 0;
    int nStreamID = 0;
    bool bIsKeyFrame = false;
    int64_t nDts=0;
    if (pPacket->bIsAudio)
    {
        nStreamType = pPacket->enAudio;
        /*音频不需要dts*/
        nDts = INT64_MIN; //PTS_NO_VALUE
    }
    else
    {
        nStreamType = pPacket->enVideo;
        bool bIsH265 = pPacket->enVideo == StreamType_E::H265;
        bIsKeyFrame = check_iFrame(pPacket->pData, pPacket->nLen, bIsH265) > 0;
    }

    auto pFind = m_streamMap.find(nStreamType);
    if (pFind == m_streamMap.end())
    {
        if (AddStream(nStreamType) < 0)
        {
            return -3;
        }
        pFind = m_streamMap.find(nStreamType);
    }
    nStreamID = pFind->second;
    int nFlags = bIsKeyFrame | MPEG_FLAG_H264_H265_WITH_AUD;
    nRet = ps_muxer_input(m_psMuxer, nStreamID, nFlags,
                          pPacket->nRtpTs, nDts,
                          pPacket->pData,
                          pPacket->nLen);
    if (nRet > 0 && nRet < m_buffer.size())
    {
        vecOutput.clear();
        vecOutput.insert(vecOutput.end(),
                         std::make_move_iterator(m_buffer.begin()),
                         std::make_move_iterator(m_buffer.begin() + nRet));
    }
    return vecOutput.size();
}

void *SIP::PS::PsEncoder::PsAllocCb(void *param, size_t bytes)
{
    auto pPsEncoder = (PsEncoder *)(param);
    if (nullptr == pPsEncoder)
    {
        return nullptr;
    }
#if MEDIA_PS_DEBUG
    MLOG_DEBUG("PS封装申请缓存[%ld]", bytes);
#endif
    /* 创建缓存时直接调用vector */
    pPsEncoder->m_buffer.clear();
    pPsEncoder->m_buffer.resize(bytes);
    return (void *)(pPsEncoder->m_buffer.data());
}

void SIP::PS::PsEncoder::PsFreeCb(void *param, void *packet)
{
}

int SIP::PS::PsEncoder::PsWriteCb(void *param, int stream, void *packet, size_t bytes)
{
    /* NOTE 封装好的PS数据会回调到这里 */
    /* NOTE 目前不在这里处理封装好的PS数据，封装好的PS数据已存在m_buffer中 */
    /* NOTE 必须返回数据大小值，回调完毕后会根据返回值进行数据操作 */

#if MEDIA_PS_DEBUG
    MLOG_DEBUG("PS封装回调数据大小[%ld]", bytes);
#endif
    return bytes;
}
