/*
 * @Author       : EasonLu
 * @Date         : 2025-03-20 16:40:24
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-05-10 11:43:29
 * @FilePath     : MediaSdp.cpp
 * @Description  : GB28181的SDP媒体信息
 */
#include "MediaSdp.h"
#include "ExternSip.h"
#include "MediaRtp.h"
#include "dlog.h"
#include "SipUtils.h"
#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#define SIP_SDP_DEBUG 0
using namespace SIP;
using namespace SIP::SDP;

VideoType_E ToSdpVideoType(const std::string &strInput);
VideoResolution_E ToSdpVideoResolution(const std::string &strInput);
VideoBitRateType_E ToSdpVideoBitRateType(const std::string &strInput);
AudioType_E ToSdpAudioType(const std::string &strInput);
AudioBit_E ToSdpAudioBit(const std::string &strInput);
AudioSampleRate_E ToSdpAudioSampleRate(const std::string &strInput);

SdpInfo_S SDP::parseSdp(const std::string &sdp)
{
    SdpInfo_S stSdp;
    {
        { /* GB28181解析 */
            stSdp.sdp = sdp;
            std::stringstream ss(sdp);
            std::string token;
            /* 解析GB28181的特有字段 */
            while (std::getline(ss, token))
            {
                auto strStart = token.substr(0, 2);
                if (strStart == "y=")
                {
                    stSdp.strSSRC = token.substr(2, 10);
#if SIP_SDP_DEBUG
                    dlog_debug("识别到字段y的SSRC[%s]", stSdp.strSSRC.c_str());
#endif
                }
                if (strStart == "f=")
                {
                    /* 去除换行 */
                    token.erase(std::remove(token.begin(), token.end(), '\r'), token.end());
                    token.erase(std::remove(token.begin(), token.end(), '\n'), token.end());
                    /* 固定格式：f=v/编码格式/分辨率/帧率/码率类型/码率大小a/编码格式/码率大小/采样率 */
                    auto pGet = token.substr(2); /* 获取为v/////a///的格式 */
                    auto posA = pGet.find('a');  /* 以a为分隔符 */
                    if (posA == std::string::npos)
                    {
#if SIP_SDP_DEBUG
                        dlog_debug("无法获取字段f格式信息[%s]", pGet.c_str());
#endif
                        continue;
                    }
                    auto strV = pGet.substr(0, posA); /* 理论上格式为:v///// */
                    auto strA = pGet.substr(posA);    /* 理论上格式为:a/// */
#if SIP_SDP_DEBUG
                    dlog_debug("识别到字段f的信息v[%s]", strV.c_str());
                    dlog_debug("识别到字段f的信息a[%s]", strA.c_str());
#endif
                    auto vecV = SplitString(strV, '/');
                    auto vecA = SplitString(strA, '/');

                    /* 已判断个数 */
                    if (6 == vecV.size())
                    {
                        /* 第0位为字段v */
                        stSdp.stVideo.enType = ToSdpVideoType(vecV[1]);
                        stSdp.stVideo.enResolution = ToSdpVideoResolution(vecV[2]);
                        if (VideoResolution_E::VR_OTHER == stSdp.stVideo.enResolution)
                        {
                            auto vecR = SplitString(vecV[2], 'x');
                            if (vecR.size() == 2)
                            {
                                SafeStr2Num(vecR[0], stSdp.stVideo.nWidth);
                                SafeStr2Num(vecR[1], stSdp.stVideo.nHeight);
                            }
                        }
                        SafeStr2Num(vecV[3], stSdp.stVideo.nFrameRate);
                        stSdp.stVideo.enBitRateType = ToSdpVideoBitRateType(vecV[4]);
                        SafeStr2Num(vecV[5], stSdp.stVideo.nBitRate);
                    }
                    else
                    {
#if SIP_SDP_DEBUG
                        dlog_debug("识别f字段中的v个数有误[%d]", vecV.size());
#endif
                    }

                    if (4 == vecA.size())
                    {
                        /* 第0位为字段a */
                        stSdp.stAudio.enType = ToSdpAudioType(vecA[1]);
                        stSdp.stAudio.enBit = ToSdpAudioBit(vecA[2]);
                        stSdp.stAudio.enSampleRate = ToSdpAudioSampleRate(vecA[3]);
                    }
                    else
                    {
#if SIP_SDP_DEBUG
                        dlog_debug("识别f字段中的a个数有误[%d]", vecA.size());
#endif
                    }
                }
            }
        }
        { /* eXosip解析 */
            sdp_message_t *pSdpMsg = nullptr;
            sdp_message_init(&pSdpMsg);
            if (pSdpMsg)
            {
                /* 不能依靠返回值判断，存在SDP报文不规范的情况 */
                sdp_message_parse(pSdpMsg, sdp.c_str());

                /* 解析对话类型 */
                std::string strSessionName = sdp_message_s_name_get(pSdpMsg);
                if (!strSessionName.empty())
                {
#if SIP_SDP_DEBUG
                    dlog_debug("识别到SDP字段s[%s]", strSessionName.c_str());
#endif
                    if (strSessionName == "Play")
                    {
                        stSdp.enSessionType = Session_E::Play;
                    }
                    if (strSessionName == "Playback")
                    {
                        stSdp.enSessionType = Session_E::Playback;
                    }
                    if (strSessionName == "Download")
                    {
                        stSdp.enSessionType = Session_E::Download;
                    }
                    if(strSessionName == "Talk")
                    {
                        stSdp.enSessionType = Session_E::Talk;
                    }
                }

                { /* 解析Playback回放时间 */
                    /* 默认拿第一个 */
                    std::string strStartTime = sdp_message_t_start_time_get(pSdpMsg, 0);
                    std::string strEndTime = sdp_message_t_stop_time_get(pSdpMsg, 0);
                    if (!strStartTime.empty() && !strEndTime.empty())
                    {
#if SIP_SDP_DEBUG
                        dlog_debug("识别到字段t的开始时间[%s],结束时间[%s]",
                                   strStartTime.c_str(),
                                   strEndTime.c_str());
#endif
                        SafeStr2Num(strStartTime, stSdp.nStartTime);
                        SafeStr2Num(strEndTime, stSdp.nEndTime);
#if SIP_SDP_DEBUG
                        dlog_debug("字段t转换后的开始时间[%lld],结束时间[%lld]",
                                   stSdp.nStartTime,
                                   stSdp.nEndTime);
#endif
                    }
                }
                auto pVideo = eXosip_get_video_media(pSdpMsg);
                auto pVideoConnection = eXosip_get_video_connection(pSdpMsg);
                /* 两者都不为空则认定为解析成功，否则失败 */
                if (pVideo && pVideoConnection)
                {
                    /* 读取payload的优先级顺序 */
                    auto pVideoPayload = (const osip_list_t *)(&pVideo->m_payloads);
                    for (int i = 0; i < osip_list_size(pVideoPayload); i++)
                    {
                        auto pPayload = (char *)osip_list_get(pVideoPayload, i);
                        if (pPayload)
                        {
#if SIP_SDP_DEBUG
                            dlog_debug("识别到视频payload[%s]", pPayload);
#endif
                            int nPayload = 0;
                            if (SafeStr2Num(std::string(pPayload), nPayload))
                            {
                                /* 插入空的数据占位 */
                                stSdp.mapVideo[nPayload] = Map_S();
                            }
                        }
                    }
                    /* 视频参数及连接相关的解析 */
                    SafeStr2Num(std::string(pVideo->m_port), stSdp.stVideoConn.nPort);
                    stSdp.stVideoConn.bIsTcp = std::string(pVideo->m_proto).find("TCP") != std::string::npos;
                    stSdp.stVideoConn.bHaveConnection = true;
                    stSdp.stVideoConn.strIP = pVideoConnection->c_addr;
                    stSdp.stVideoConn.bIsIPV6 = std::string(pVideoConnection->c_addrtype).find("6") != std::string::npos;
                    stSdp.mediaType = AV_VIDEO;  //add by longll
                    auto pVidoeAttr = (const osip_list_t *)(&pVideo->a_attributes);
                    /* 解析rtpmap,格式:[a=rtpmap:96 PS/90000] */
                    for (int i = 0; i < osip_list_size(pVidoeAttr); i++)
                    {
                        auto pAttr = (sdp_attribute *)osip_list_get(pVidoeAttr, i);
                        if (pAttr)
                        {
#if SIP_SDP_DEBUG
                            dlog_debug("识别到a字段[%s][%s]",
                                       pAttr->a_att_field, pAttr->a_att_value);
#endif
                            if (std::string(pAttr->a_att_field).find("rtpmap") != std::string::npos)
                            {
#if SIP_SDP_DEBUG
                                dlog_debug("rtpmap字段[%s]", pAttr->a_att_value);
#endif
                                Map_S stRtpmap;
                                auto vecRtpmap = SplitString(pAttr->a_att_value, ' ');
                                if (vecRtpmap.size() == 2)
                                {
                                    SafeStr2Num(vecRtpmap[0], stRtpmap.nPayloadType);
                                    auto vecType = SplitString(vecRtpmap[1], '/');
                                    if (vecType.size() == 2)
                                    {
                                        stRtpmap.strCodecName = vecType[0];
                                        SafeStr2Num(vecType[1], stRtpmap.nClockRate);
                                        /* 匹配到识别的优先级顺序才更新 */
                                        if (stSdp.mapVideo.find(stRtpmap.nPayloadType) != stSdp.mapVideo.end())
                                        {
                                            stSdp.mapVideo[stRtpmap.nPayloadType] = stRtpmap;
                                        }
                                    }
                                }
                            }
                            else if (std::string(pAttr->a_att_field).find("setup") != std::string::npos)
                            {
                                if (std::string(pAttr->a_att_value).find("active") != std::string::npos)
                                {
                                    stSdp.stVideoConn.bTcpActive = true;
                                }
                            }
                            else if (std::string(pAttr->a_att_field).find("recvonly") != std::string::npos)
                            {
                                /* 解析处理recvonly字段 */
                                stSdp.stVideoConn.bRecvOnly = true;
                            }
                            else if (std::string(pAttr->a_att_field).find("downloadspeed") != std::string::npos)
                            {
                                /* 解析下载速度字段 */
                                SafeStr2Num(pAttr->a_att_value, stSdp.nDownloadSpeed);
                            }
                        }
                    }
                }

                /* TODO 音频解析后续补充 */
                auto pAudio = eXosip_get_audio_media(pSdpMsg);
                auto pAudioConnection = eXosip_get_audio_connection(pSdpMsg);
                if (pAudio && pAudioConnection)
                {
                    /* 读取payload的优先级顺序 */
                    auto pAudioPayload = (const osip_list_t *)(&pAudio->m_payloads);
                    for (int i = 0; i < osip_list_size(pAudioPayload); i++)
                    {
                        auto pPayload = (char *)osip_list_get(pAudioPayload, i);
                        if (pPayload)
                        {
#if SIP_SDP_DEBUG
                            dlog_debug("识别到视频payload[%s]", pPayload);
#endif
                            int nPayload = 0;
                            if (SafeStr2Num(std::string(pPayload), nPayload))
                            {
                                /* 插入空的数据占位 */
                                stSdp.mapAudio[nPayload] = Map_S();
                            }
                        }
                    }
                    /* 音频参数及连接相关的解析 */
                    // SafeStr2Num(std::string(pAudio->m_port), stSdp.stAudioConn.nPort);
                    stSdp.stAudioConn.bIsTcp = std::string(pAudio->m_proto).find("TCP") != std::string::npos;
                    stSdp.stAudioConn.bHaveConnection = true;
                    stSdp.stAudioConn.strIP = sdp_message_o_addr_get(pSdpMsg);
                    stSdp.stAudioConn.nPort = atoi(sdp_message_m_port_get(pSdpMsg, 0)); 
                    stSdp.stAudioConn.bIsIPV6 = std::string(pAudioConnection->c_addrtype).find("6") != std::string::npos;
                    stSdp.mediaType = AV_AUDIO;  //add by longll
                    auto pAudioAttr = (const osip_list_t *)(&pAudio->a_attributes);
                    /* 解析rtpmap,格式:[a=rtpmap:96 PS/90000] */
                    for (int i = 0; i < osip_list_size(pAudioAttr); i++)
                    {
                        auto pAttr = (sdp_attribute *)osip_list_get(pAudioAttr, i);
                        if (pAttr)
                        {
#if SIP_SDP_DEBUG
                            dlog_debug("识别到a字段[%s][%s]",
                                       pAttr->a_att_field, pAttr->a_att_value);
#endif
                            if (std::string(pAttr->a_att_field).find("rtpmap") != std::string::npos)
                            {
#if SIP_SDP_DEBUG
                                dlog_debug("rtpmap字段[%s]", pAttr->a_att_value);
#endif
                                Map_S stRtpmap;
                                auto vecRtpmap = SplitString(pAttr->a_att_value, ' ');
                                if (vecRtpmap.size() == 2)
                                {
                                    SafeStr2Num(vecRtpmap[0], stRtpmap.nPayloadType);
                                    auto vecType = SplitString(vecRtpmap[1], '/');
                                    if (vecType.size() == 2)
                                    {
                                        stRtpmap.strCodecName = vecType[0];
                                        SafeStr2Num(vecType[1], stRtpmap.nClockRate);
                                        /* 匹配到识别的优先级顺序才更新 */
                                        if (stSdp.mapAudio.find(stRtpmap.nPayloadType) != stSdp.mapAudio.end())
                                        {
                                            stSdp.mapAudio[stRtpmap.nPayloadType] = stRtpmap;
                                        }
                                    }
                                }
                            }
                            else if (std::string(pAttr->a_att_field).find("setup") != std::string::npos)
                            {
                                if (std::string(pAttr->a_att_value).find("active") != std::string::npos)
                                {
                                    stSdp.stAudioConn.bTcpActive = true;
                                }
                            }
                            else if (std::string(pAttr->a_att_field).find("recvonly") != std::string::npos)
                            {
                                /* 解析处理recvonly字段 */
                                stSdp.stAudioConn.bRecvOnly = true;
                            }
                            else if (std::string(pAttr->a_att_field).find("downloadspeed") != std::string::npos)
                            {
                                /* 解析下载速度字段 */
                                SafeStr2Num(pAttr->a_att_value, stSdp.nDownloadSpeed);
                            }
                            else if (std::string(pAttr->a_att_field).find("sendonly") != std::string::npos)
                            {
                                /* 解析处理sendonly字段 */
                                stSdp.stAudioConn.bRecvOnly = false;
                            }
                        }
                    }
                }

                sdp_message_free(pSdpMsg);
            }
        }
    }

    return stSdp;
}

std::string SIP::SDP::negotiateSdp(
    const SdpInfo_S &remoteSdp,
    SdpNegotiate_S &negInfo)
{

    /* 记录PS的Rtpmap */
    int nPsPayload = 0;
    for (auto &&it : remoteSdp.mapVideo)
    {
        if (it.second.strCodecName == RTPMAP_PS_CODEC)
        {
            nPsPayload = it.first;
            break;
        }
    }
    /*
        根据SDP（Session Description Protocol）的规范，SDP 中的各个字段其实是没有顺序要求的。
        每一行的含义都是独立的，可以按任意顺序排列。
        但是，为了方便阅读和解析，通常会按照规定的顺序来排列。
            v= (协议版本)
            o= (创建者和会话标识符)
            s= (会话名称)
            i=* (会话信息)
            u=* (描述的 URI)
            e=* (电子邮件地址)
            p=* (电话号码)
            c=* (连接信息 - 如果在所有媒体中都包含则不需要)
            b=* (带宽信息)
            t=* (会话起止时间)
            r=* (重复时间)
            z=* (时区调整)
            k=* (加密密钥)
            a=* (零个或多个会话属性行)
            Zero or more media descriptions
    */
    std::stringstream ss;
    ss << "v=0\r\n";
    ss << "o=" << negInfo.strID << " 25 25 IN IP4 " << negInfo.strIP << "\r\n";
    if (Session_E::Play == remoteSdp.enSessionType)
    {
        /* 实时点播 */
        ss << "s=Play\r\n";
    }
    else if (Session_E::Talk == remoteSdp.enSessionType)
    {
         /* 语音对讲--兼容GB28181-2016版 */
         ss << "s=Talk\r\n";
    }
    else if (Session_E::Playback == remoteSdp.enSessionType)
    {
        /* 历史回放 */
        ss << "s=Playback\r\n";
    }
    else if (Session_E::Download == remoteSdp.enSessionType)
    {
        /* 文件下载 */
        ss << "s=Download\r\n";
    }
    ss << "c=IN IP4 " << negInfo.strIP << "\r\n";
    /* 历史回放的会话类型会用到此字段，实时点播默认给0即可 */
    ss << "t=" << negInfo.nStartTime << " " << negInfo.nEndTime << "\r\n";
    /* NOTE RTP/AVP为默认UDP协议，TCP/RTP/AVP为TCP协议，只采用PS封装格式 */
    { /* 拼接视频字段 */
        ss << "m=video ";
        ss << negInfo.nPort << " "; /* 保留这个空格 */

        if (remoteSdp.stVideoConn.bIsTcp)
        {
            ss << "TCP/";
        }
        /* 协商只保留PS格式的Payload */
        ss << "RTP/AVP " << nPsPayload << "\r\n";
        /* 更新协商后的SDP */
        negInfo.stVideo.nPayload = nPsPayload;
    }
    dlog_error("remoteSdp.mediaType ==%d", remoteSdp.mediaType);
    ss << "a=sendonly\r\n"; /* 仅发送，不接收数据 */
    if (remoteSdp.stVideoConn.bTcpActive)
    {
        /* 远端设置了TCP主动，协商参数需要填写TCP被动 */
        ss << "a=setup:passive\r\n";
    }
    else
    {
        /* TCP协议必须设置为被动连接，否则无法播放 */
        ss << "a=setup:active\r\n";
    }
    ss << "a=connection:new\r\n"; /* 开启新连接，不复用旧连接 */
    if (nPsPayload)
    {
        ss << "a=rtpmap:" << nPsPayload << " " << remoteSdp.mapVideo.at(nPsPayload).strCodecName << "/" << remoteSdp.mapVideo.at(nPsPayload).nClockRate << "\r\n";
    }
    ss << "y=" << remoteSdp.strSSRC << "\r\n";
    { /* 固定格式：f=v/编码格式/分辨率/帧率/码率类型/码率大小a/编码格式/码率大小/采样率 */
        ss << "f=";
        { /* 视频 */
            ss << "v/";
            {
                if (negInfo.stVideo.enType > 0)
                {
                    ss << negInfo.stVideo.enType;
                }
                ss << "/";
            }
            {
                if (negInfo.stVideo.enResolution == VideoResolution_E::VR_OTHER)
                {
                    if (negInfo.stVideo.nWidth > 0 && negInfo.stVideo.nHeight > 0)
                    {
                        ss << negInfo.stVideo.nWidth << "x" << negInfo.stVideo.nHeight;
                    }
                }
                else if (negInfo.stVideo.enResolution > 0)
                {
                    ss << negInfo.stVideo.enResolution;
                }
                ss << "/";
            }
            {
                if (negInfo.stVideo.nFrameRate > 0)
                {
                    ss << negInfo.stVideo.nFrameRate;
                }
                ss << "/";
            }
            {
                if (negInfo.stVideo.enBitRateType > 0)
                {
                    ss << negInfo.stVideo.enBitRateType;
                }
                ss << "/";
            }
            {
                if (negInfo.stVideo.nBitRate > 0)
                {
                    ss << negInfo.stVideo.nBitRate;
                }
                //ss << "/";
            }
        }

        { /* 音频 */
            ss << "a/";
            {
                if (negInfo.stAudio.enType > 0)
                {
                    ss << negInfo.stAudio.enType;
                }
                ss << "/";
            }
            {
                if (negInfo.stAudio.enBit > 0)
                {
                    ss << negInfo.stAudio.enBit;
                }
                ss << "/";
            }
            {
                if (negInfo.stAudio.enSampleRate > 0)
                {
                    ss << negInfo.stAudio.enSampleRate;
                }
                ss << "/";
            }
        }
        ss << "\r\n";
    }

    return ss.str();
}

//add by longll
std::string negotiateSdpaudio(
    const SdpInfo_S &remoteSdp,
    SdpNegotiate_S &negInfo)
{

    /* 记录PS的Rtpmap */
    int nPsPayload = 0;
    for (auto &&it : remoteSdp.mapVideo)
    {
        if (it.second.strCodecName == RTPMAP_PS_CODEC)
        {
            nPsPayload = it.first;
            break;
        }
    }

    /* 记录PS的Rtpmap */
    int nAudioPsPayload = 0;
    for (auto &&it : remoteSdp.mapAudio)
    {
        if (it.second.strCodecName == RTPMAP_PS_CODEC)
        {
            nAudioPsPayload = it.first;
            break;
        }
    }
    /*
        根据SDP（Session Description Protocol）的规范，SDP 中的各个字段其实是没有顺序要求的。
        每一行的含义都是独立的，可以按任意顺序排列。
        但是，为了方便阅读和解析，通常会按照规定的顺序来排列。
            v= (协议版本)
            o= (创建者和会话标识符)
            s= (会话名称)
            i=* (会话信息)
            u=* (描述的 URI)
            e=* (电子邮件地址)
            p=* (电话号码)
            c=* (连接信息 - 如果在所有媒体中都包含则不需要)
            b=* (带宽信息)
            t=* (会话起止时间)
            r=* (重复时间)
            z=* (时区调整)
            k=* (加密密钥)
            a=* (零个或多个会话属性行)
            Zero or more media descriptions
    */
    std::stringstream ss;
    ss << "v=0\r\n";
    ss << "o=" << negInfo.strID << " 25 25 IN IP4 " << negInfo.strIP << "\r\n";
    if (Session_E::Play == remoteSdp.enSessionType)
    {
        /* 实时点播 */
        ss << "s=Play\r\n";
    }
    else if (Session_E::Talk == remoteSdp.enSessionType)
    {
         /* 语音对讲--兼容GB28181-2016版 */
         ss << "s=Talk\r\n";
    }
    else if (Session_E::Playback == remoteSdp.enSessionType)
    {
        /* 历史回放 */
        ss << "s=Playback\r\n";
    }
    else if (Session_E::Download == remoteSdp.enSessionType)
    {
        /* 文件下载 */
        ss << "s=Download\r\n";
    }
    ss << "c=IN IP4 " << negInfo.strIP << "\r\n";
    /* 历史回放的会话类型会用到此字段，实时点播默认给0即可 */
    ss << "t=" << negInfo.nStartTime << " " << negInfo.nEndTime << "\r\n";
    /* NOTE RTP/AVP为默认UDP协议，TCP/RTP/AVP为TCP协议，只采用PS封装格式 */
    
    if(remoteSdp.mediaType == AV_VIDEO)
    { /* 拼接视频字段 */
        ss << "m=video ";
        ss << negInfo.nPort << " "; /* 保留这个空格 */

        if (remoteSdp.stVideoConn.bIsTcp)
        {
            ss << "TCP/";
        }
        /* 协商只保留PS格式的Payload */
        ss << "RTP/AVP " << nPsPayload << "\r\n";
        /* 更新协商后的SDP */
        negInfo.stVideo.nPayload = nPsPayload;
    }
    else if(remoteSdp.mediaType == AV_AUDIO)
    {
        ss << "m=audio ";
        ss << negInfo.nPort << " "; /* 保留这个空格 */

        if (remoteSdp.stVideoConn.bIsTcp)
        {
            ss << "TCP/";
        }
        /* 协商只保留PS格式的Payload */
        ss << "RTP/AVP " << nAudioPsPayload << "\r\n";
        /* 更新协商后的SDP */
        negInfo.stVideo.nPayload = nAudioPsPayload;
    }
    dlog_error("remoteSdp.mediaType ==%d", remoteSdp.mediaType);
    ss << "a=sendonly\r\n"; /* 仅发送，不接收数据 */
    if (remoteSdp.stVideoConn.bTcpActive)
    {
        /* 远端设置了TCP主动，协商参数需要填写TCP被动 */
        ss << "a=setup:passive\r\n";
    }
    else
    {
        /* TCP协议必须设置为被动连接，否则无法播放 */
        ss << "a=setup:active\r\n";
    }
    ss << "a=connection:new\r\n"; /* 开启新连接，不复用旧连接 */
    if (nPsPayload)
    {
        //ss << "a=rtpmap:" << nPsPayload << " " << remoteSdp.mapVideo.at(nPsPayload).strCodecName << "/" << remoteSdp.mapVideo.at(nPsPayload).nClockRate << "\r\n";
        if(remoteSdp.mediaType == AV_VIDEO)
        {
            ss << "a=rtpmap:" << nPsPayload << " " << remoteSdp.mapVideo.at(nPsPayload).strCodecName << "/" << remoteSdp.mapVideo.at(nPsPayload).nClockRate << "\r\n";
        }
        else if(remoteSdp.mediaType == AV_AUDIO)
        {
            ss << "a=rtpmap:" << nPsPayload << " " << remoteSdp.mapAudio.at(nPsPayload).strCodecName << "/" << remoteSdp.mapAudio.at(nPsPayload).nClockRate << "\r\n";
        }
       
    }
    ss << "y=" << remoteSdp.strSSRC << "\r\n";
    { /* 固定格式：f=v/编码格式/分辨率/帧率/码率类型/码率大小a/编码格式/码率大小/采样率 */
        ss << "f=";
        { /* 视频 */
            ss << "v/";
            {
                if (negInfo.stVideo.enType > 0)
                {
                    ss << negInfo.stVideo.enType;
                }
                ss << "/";
            }
            {
                if (negInfo.stVideo.enResolution == VideoResolution_E::VR_OTHER)
                {
                    if (negInfo.stVideo.nWidth > 0 && negInfo.stVideo.nHeight > 0)
                    {
                        ss << negInfo.stVideo.nWidth << "x" << negInfo.stVideo.nHeight;
                    }
                }
                else if (negInfo.stVideo.enResolution > 0)
                {
                    ss << negInfo.stVideo.enResolution;
                }
                ss << "/";
            }
            {
                if (negInfo.stVideo.nFrameRate > 0)
                {
                    ss << negInfo.stVideo.nFrameRate;
                }
                ss << "/";
            }
            {
                if (negInfo.stVideo.enBitRateType > 0)
                {
                    ss << negInfo.stVideo.enBitRateType;
                }
                ss << "/";
            }
            {
                if (negInfo.stVideo.nBitRate > 0)
                {
                    ss << negInfo.stVideo.nBitRate;
                }
                //ss << "/";
            }
        }

        { /* 音频 */
            ss << "a/";
            {
                if (negInfo.stAudio.enType > 0)
                {
                    ss << negInfo.stAudio.enType;
                }
                ss << "/";
            }
            {
                if (negInfo.stAudio.enBit > 0)
                {
                    ss << negInfo.stAudio.enBit;
                }
                ss << "/";
            }
            {
                if (negInfo.stAudio.enSampleRate > 0)
                {
                    ss << negInfo.stAudio.enSampleRate;
                }
                ss << "/";
            }
        }
        ss << "\r\n";
    }

    return ss.str();
}

VideoType_E ToSdpVideoType(const std::string &strInput)
{
    auto enRet = VideoType_E::VT_NONE;
    if (strInput.empty())
    {
        return enRet;
    }
    int nVideoType = 0;
    if (SafeStr2Num(strInput, nVideoType))
    {
        if (nVideoType >= VideoType_E::VT_MPEG4 &&
            nVideoType <= VideoType_E::VT_H265)
        {
            enRet = static_cast<VideoType_E>(nVideoType);
        }
    }
    return enRet;
}

VideoResolution_E ToSdpVideoResolution(const std::string &strInput)
{
    auto enRet = VideoResolution_E::VR_NONE;
    if (strInput.empty())
    {
        return enRet;
    }
    /* 自定义分辨率带有x，用x分割宽高 */
    if (strInput.find("x") != std::string::npos)
    {
        return VideoResolution_E::VR_OTHER;
    }
    int nVideoResolution = 0;
    if (SafeStr2Num(strInput, nVideoResolution))
    {
        if (nVideoResolution >= VideoResolution_E::VR_QCIF &&
            nVideoResolution <= VideoResolution_E::VR_1080PI)
        {
            enRet = static_cast<VideoResolution_E>(nVideoResolution);
        }
    }
    return enRet;
}

VideoBitRateType_E ToSdpVideoBitRateType(const std::string &strInput)
{
    auto enRet = VideoBitRateType_E::VBRT_NONE;
    if (strInput.empty())
    {
        return enRet;
    }
    int nVideoBitRateType = 0;
    if (SafeStr2Num(strInput, nVideoBitRateType))
    {
        switch (nVideoBitRateType)
        {
        case VideoBitRateType_E::VBRT_CBR:
            enRet = VideoBitRateType_E::VBRT_CBR;
            break;
        case VideoBitRateType_E::VBRT_VBR:
            enRet = VideoBitRateType_E::VBRT_VBR;
            break;
        default:
            break;
        }
    }
    return enRet;
}

AudioType_E ToSdpAudioType(const std::string &strInput)
{
    auto enRet = AudioType_E::AT_NONE;
    if (strInput.empty())
    {
        return enRet;
    }
    int nAudioType = 0;
    if (SafeStr2Num(strInput, nAudioType))
    {
        if (nAudioType >= AudioType_E::AT_G711 &&
            nAudioType <= AudioType_E::AT_AAC)
        {
            enRet = static_cast<AudioType_E>(nAudioType);
        }
    }
    return enRet;
}

AudioBit_E ToSdpAudioBit(const std::string &strInput)
{
    auto enRet = AudioBit_E::AB_NONE;
    if (strInput.empty())
    {
        return enRet;
    }
    int nAudioBit = 0;
    if (SafeStr2Num(strInput, nAudioBit))
    {
        if (nAudioBit >= AudioBit_E::AB_5_3 &&
            nAudioBit <= AudioBit_E::AB_48_61)
        {
            enRet = static_cast<AudioBit_E>(nAudioBit);
        }
    }
    return enRet;
}

AudioSampleRate_E ToSdpAudioSampleRate(const std::string &strInput)
{
    auto enRet = AudioSampleRate_E::AS_NONE;
    if (strInput.empty())
    {
        return enRet;
    }
    int nAudioSampleRate = 0;
    if (SafeStr2Num(strInput, nAudioSampleRate))
    {
        if (nAudioSampleRate >= AudioSampleRate_E::AS_8 &&
            nAudioSampleRate <= AudioSampleRate_E::AS_38_4)
        {
            enRet = static_cast<AudioSampleRate_E>(nAudioSampleRate);
        }
    }
    return enRet;
}
