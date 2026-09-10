/*
 * @Author       : EasonLu
 * @Date         : 2025-03-20 16:40:19
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-04-22 14:29:43
 * @FilePath     : MediaSdp.h
 * @Description  : GB28181的SDP媒体信息
 * @Note         : 参考GB/T 28181-2022 附录G SDP定义 p129
 */
#pragma once
#include "SipType.h"
#include <map>
#include <string>

namespace SIP
{
    namespace SDP
    {
        typedef struct _Map_S_
        {
            int nPayloadType;         /* 编码器代号 */
            std::string strCodecName; /* 编码器名称 */
            int nClockRate;           /* 时钟基准 */
            int nChannel;             /* 音频通道数 */
            _Map_S_()
            {
                nPayloadType = 0;
                strCodecName = "";
                nClockRate = 0;
                nChannel = 0;
            }
            /* 重载赋值运算符 */
            _Map_S_ &operator=(const _Map_S_ &stInfo)
            {
                if (this != &stInfo)
                {
                    nPayloadType = stInfo.nPayloadType;
                    strCodecName = stInfo.strCodecName;
                    nClockRate = stInfo.nClockRate;
                    nChannel = stInfo.nChannel;
                }
                return *this;
            }
        } Map_S;

        /* NOTE 枚举值的顺序不能调乱，且不能随意增删，要参考GB/T 28181-2022文档 */

        typedef enum _Session_E_
        {
            Play = 0, /* 实时点播 */
            Playback, /* 历史回放 */
            Download, /* 文件下载 */
            Talk,     /* 语音对讲*/
            Broadcast,/* 广播 */
        } Session_E;

        typedef enum _VideoType_E_
        {
            VT_NONE = 0,  /* 无视频 */
            VT_MPEG4 = 1, /* MPEG4 */
            VT_H264 = 2,  /* H264 */
            VT_SVAC = 3,  /* SVAC */
            VT_3GP = 4,   /* 3GP */
            VT_H265 = 5,  /* H265 */
        } VideoType_E;

        typedef enum _VideoResolution_E_
        {
            VR_NONE = 0,
            VR_QCIF = 1,
            VR_CIF = 2,
            VR_4CIF = 3,
            VR_D1 = 4,
            VR_720P = 5,
            VR_1080PI = 6,
            VR_OTHER = 7, /* 自定义分辨率 */
        } VideoResolution_E;

        typedef enum _VideoBitRateType_E_
        {
            VBRT_NONE = 0,
            VBRT_CBR = 1, /* 固定码率 */
            VBRT_VBR = 2, /* 可变码率 */
        } VideoBitRateType_E;

        typedef struct _VideoInfo_S_
        {
            VideoType_E enType;             /* 编码格式 */
            VideoResolution_E enResolution; /* 为OTHER时用WxH表示 */
            int nWidth;                     /* enResolution为OTHER时有效 */
            int nHeight;                    /* enResolution为OTHER时有效 */
            int nFrameRate;
            int nBitRate; /* 码率[0~100000]单位[kb/s] */
            VideoBitRateType_E enBitRateType;
            int nPayload;
            _VideoInfo_S_()
            {
                enType = VT_NONE;
                enResolution = VR_NONE;
                nWidth = 0;
                nHeight = 0;
                nFrameRate = 0;
                nBitRate = 0;
                enBitRateType = VBRT_NONE;
                nPayload = 0;
            }
            /* 重载赋值运算符 */
            _VideoInfo_S_ &operator=(const _VideoInfo_S_ &stInfo)
            {
                if (this != &stInfo)
                {
                    enType = stInfo.enType;
                    enResolution = stInfo.enResolution;
                    nWidth = stInfo.nWidth;
                    nHeight = stInfo.nHeight;
                    nFrameRate = stInfo.nFrameRate;
                    nBitRate = stInfo.nBitRate;
                    enBitRateType = stInfo.enBitRateType;
                    nPayload = stInfo.nPayload;
                }
                return *this;
            }
        } VideoInfo_S;

        typedef enum _AudioType_E_
        {
            AT_NONE = 0, /* 无音频 */
            AT_G711 = 1, /* G711 */
            AT_G723 = 2, /* G723 */
            AT_G729 = 3, /* G729 */
            AT_G722 = 4, /* G722 */
            AT_SVAC = 5, /* SVAC */
            AT_AAC = 6,  /* AAC */
        } AudioType_E;

        /*
            注1:G.723.1中使用1、2.
            注2:G.722.1中使用4、5、6、7.
            注3:G.729中使用3.
            注4:G.711中使用8.
            注5:SVAC中使用5、9、20、21、22、23、24、25、26、27、28、29、30.
            注6:AAC中使用4、5、6、7、8、9、10、11、12、13、14、15、16、17、18、19。
         */
        typedef enum _AudioBit_E_
        {
            /* 音频码率已 '_' 为小数点，单位为[kb/s] */
            AB_NONE = 0,
            AB_5_3 = 1,
            AB_6_3 = 2,
            AB_8 = 3,
            AB_16 = 4,
            AB_24 = 5,
            AB_32 = 6,
            AB_48 = 7,
            AB_64 = 8,
            AB_12 = 9,
            AB_80 = 10,
            AB_96 = 11,
            AB_112 = 12,
            AB_128 = 13,
            AB_160 = 14,
            AB_192 = 15,
            AB_224 = 16,
            AB_256 = 17,
            AB_288 = 18,
            AB_320 = 19,
            AB_10_8 = 20,
            AB_12_4 = 21,
            AB_14 = 22,
            AB_15_6 = 23,
            AB_17_2 = 24,
            AB_19_6 = 25,
            AB_21_2 = 26,
            AB_24_4 = 27,
            AB_23_05 = 28,
            AB_34 = 29,
            AB_48_61 = 30,
        } AudioBit_E;

        /*
            注7:G.711中使用1.
            注8:G.722.1中使用2、3、4.
            注9:G.723.1中使用1.
            注10:G.729中使用1.
            注11:SVAC中使用3、4、9、11,15、16、17.
            注12:AAC中使用1、3、4、5、6、7、8、9、10、11、12、13、14。
        */
        typedef enum _AudioSampleRate_E_
        {
            /* 音频采样率已 '_' 为小数点，单位为[kHz] */
            AS_NONE = 0,
            AS_8 = 1,
            AS_14 = 2,
            AS_16 = 3,
            AS_32 = 4,
            AS_7 = 5,
            AS_11 = 6,
            AS_12 = 7,
            AS_22 = 8,
            AS_24 = 9,
            AS_44 = 10,
            AS_48 = 11,
            AS_64 = 12,
            AS_88 = 13,
            AS_96 = 14,
            AS_12_8 = 15,
            AS_25_6 = 16,
            AS_38_4 = 17,
        } AudioSampleRate_E;

        typedef enum _MediaType_E_  //add by longll
        {
            /* m字段是video或audio */
            AV_NONE = 0,
            AV_VIDEO = 1,
            AV_AUDIO = 2,
        } MediaType_E;

        typedef struct _AudioInfo_S
        {
            AudioType_E enType;
            AudioBit_E enBit;
            AudioSampleRate_E enSampleRate;
            int nPayload;
            _AudioInfo_S()
            {
                enType = AudioType_E::AT_NONE;
                enBit = AudioBit_E::AB_NONE;
                enSampleRate = AudioSampleRate_E::AS_NONE;
                nPayload = 0;
            }
            /* 重载赋值运算符 */
            _AudioInfo_S &operator=(const _AudioInfo_S &stInfo)
            {
                if (this != &stInfo)
                {
                    enType = stInfo.enType;
                    enBit = stInfo.enBit;
                    enSampleRate = stInfo.enSampleRate;
                    nPayload = stInfo.nPayload;
                }
                return *this;
            }
        } AudioInfo_S;

        /// @brief 连接信息
        typedef struct _ConnectionInfo_S_
        {
            bool bHaveConnection;
            std::string strIP;
            int nPort;
            bool bIsTcp;
            bool bIsIPV6;
            bool bTcpActive; /* TCP主动标记位(默认被动) */
            bool bRecvOnly;  /* 接收流标记位(默认为发送) */
            _ConnectionInfo_S_()
            {
                bHaveConnection = false;
                strIP = "";
                nPort = 0;
                bIsTcp = false;
                bIsIPV6 = false;
                bTcpActive = false;
                bRecvOnly = false;
            }
            /* 重载赋值运算符 */
            _ConnectionInfo_S_ &operator=(const _ConnectionInfo_S_ &stInfo)
            {
                if (this != &stInfo)
                {
                    bHaveConnection = stInfo.bHaveConnection;
                    strIP = stInfo.strIP;
                    nPort = stInfo.nPort;
                    bIsTcp = stInfo.bIsTcp;
                    bIsIPV6 = stInfo.bIsIPV6;
                    bTcpActive = stInfo.bTcpActive;
                    bRecvOnly = stInfo.bRecvOnly;
                }
                return *this;
            }
        } ConnectionInfo_S;

        /// @brief 解析GB28181所需的字段的SDP字段
        typedef struct _SDP_S_
        {
            std::string sdp;     /* 原始报文 */
            std::string strSSRC; /* 字段y中存放十位的SSRC */
            /* 格式：f=v/编码格式/分辨率/帧率/码率类型/码率大小a/编码格式/码率大小/采样率 */
            /* 字段f中的音视频编码数据 */
            VideoInfo_S stVideo;
            AudioInfo_S stAudio;

            /* 音视频的连接信息 */
            ConnectionInfo_S stVideoConn;
            ConnectionInfo_S stAudioConn;

            /* rtpmap信息 */
            std::map<int, Map_S> mapVideo;
            std::map<int, Map_S> mapAudio;

            /* 当前会话类型 */
            Session_E enSessionType;
            /* 回放和下载的时间段 */
            uint64_t nStartTime;
            uint64_t nEndTime;
            /* 下载速度 */
            int nDownloadSpeed;

            /*媒体类型*/
            MediaType_E mediaType;  //add by longll
            _SDP_S_()
            {
                sdp = "";
                strSSRC = "";
                stVideo = VideoInfo_S();
                stAudio = AudioInfo_S();
                stVideoConn = ConnectionInfo_S();
                stAudioConn = ConnectionInfo_S();
                mapVideo.clear();
                mapAudio.clear();
                enSessionType = Session_E::Play; /* 默认点播 */
                nStartTime = 0;
                nEndTime = 0;
                nDownloadSpeed = 0;
            }
            _SDP_S_ &operator=(const _SDP_S_ &stInfo)
            {
                if (this != &stInfo)
                {
                    sdp = stInfo.sdp;
                    strSSRC = stInfo.strSSRC;
                    stVideo = stInfo.stVideo;
                    stAudio = stInfo.stAudio;
                    stVideoConn = stInfo.stVideoConn;
                    stAudioConn = stInfo.stAudioConn;
                    mapVideo = stInfo.mapVideo;
                    mapAudio = stInfo.mapAudio;
                    enSessionType = stInfo.enSessionType;
                    nStartTime = stInfo.nStartTime;
                    nEndTime = stInfo.nEndTime;
                    nDownloadSpeed = stInfo.nDownloadSpeed;
                    mediaType = stInfo.mediaType;
                }
                return *this;
            }
        } SdpInfo_S;

        /// @brief 协商SDP时本地所需的数据
        typedef struct _Sdp_Negotiate_S_
        {
            std::string strID;
            std::string strIP;
            int nPort;
            uint64_t nStartTime;
            uint64_t nEndTime;

            VideoInfo_S stVideo;
            AudioInfo_S stAudio;
            _Sdp_Negotiate_S_()
            {
                strID = "";
                strIP = "";
                nPort = 0;
                nStartTime = 0;
                nEndTime = 0;
                stVideo = VideoInfo_S();
                stAudio = AudioInfo_S();
            }
            _Sdp_Negotiate_S_ &operator=(const _Sdp_Negotiate_S_ &stInfo)
            {
                if (this != &stInfo)
                {
                    strID = stInfo.strID;
                    strIP = stInfo.strIP;
                    nPort = stInfo.nPort;
                    nStartTime = stInfo.nStartTime;
                    nEndTime = stInfo.nEndTime;
                    stVideo = stInfo.stVideo;
                    stAudio = stInfo.stAudio;
                }
                return *this;
            }
        } SdpNegotiate_S;

        SdpInfo_S parseSdp(const std::string &sdp);

        std::string negotiateSdp(const SdpInfo_S &remoteSdp, SdpNegotiate_S &negInfo);
    }
}