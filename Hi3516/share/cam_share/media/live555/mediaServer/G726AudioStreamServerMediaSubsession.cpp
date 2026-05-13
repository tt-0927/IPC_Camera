/**
 * @FilePath     : G726AudioStreamServerMediaSubsession.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-09 17:16:23
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-06-10 14:08:10
 * @Description  : G.726音频流RTSP媒体子会话管理类实现
 * 实现G.726音频格式的RTSP流媒体服务功能，包括音频数据源创建、RTP封装器配置、以及音频参数管理等核心功能。
 * 
 * 实现要点：
 * 1. createNewStreamSource(): 创建G726AudioStreamSource，验证音频参数合法性
 * 2. createNewRTPSink(): 根据位深度选择正确的G.726 MIME类型和RTP封装
 * 3. 支持动态负载类型，适应不同的G.726变体
 * 4. 正确计算G.726格式的比特率和帧参数
 */

#include "G726AudioStreamServerMediaSubsession.h"
#include "G726AudioStreamSource.h"
#include "SimpleRTPSink.hh"

G726AudioStreamServerMediaSubsession* G726AudioStreamServerMediaSubsession
::createNew(UsageEnvironment& env, Boolean reuseFirstSource, Audio_Source_Info_t& stuG726) {
    return new G726AudioStreamServerMediaSubsession(env, reuseFirstSource, stuG726);
}

G726AudioStreamServerMediaSubsession
::G726AudioStreamServerMediaSubsession(UsageEnvironment& env, Boolean reuseFirstSource, Audio_Source_Info_t& stuG726)
    : OnDemandServerMediaSubsession(env, reuseFirstSource){

    memcpy(&m_stuG726SourceInfo, &stuG726, sizeof(Audio_Source_Info_t));

}

G726AudioStreamServerMediaSubsession
::~G726AudioStreamServerMediaSubsession() {
}

FramedSource* G726AudioStreamServerMediaSubsession
::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) {
    FramedSource* resultSource = NULL;
    estBitrate = 32; // G.726默认32kbps
    do {
        G726AudioStreamSource* g726Source
            = G726AudioStreamSource::createNew(envir(), m_stuG726SourceInfo);
        if (g726Source == NULL) break;

        // 获取音频源的属性
        fBitsPerSample = g726Source->bitsPerSample();
        // G.726支持2, 3, 4, 5位/样本（对应16, 24, 32, 40 kbps）
        if (!(fBitsPerSample == 2 || fBitsPerSample == 3 || fBitsPerSample == 4 || fBitsPerSample == 5)) {
            envir() << "The G.726 source contains " << fBitsPerSample << " bit-per-sample audio, which is not supported (should be 2, 3, 4, or 5)\n";
            break;
        }

        fSamplingFrequency = g726Source->samplingFrequency();
        fNumChannels = g726Source->numChannels();

        // G.726比特率计算：采样率 × 位深度 × 通道数
        unsigned bitsPerSecond = fSamplingFrequency * fBitsPerSample * fNumChannels;
        estBitrate = (bitsPerSecond + 500) / 1000; // kbps

        resultSource = g726Source;
        return resultSource;
    } while (0);

    Medium::close(resultSource);
    return NULL;
}

RTPSink* G726AudioStreamServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock,
        unsigned char rtpPayloadTypeIfDynamic,
        FramedSource* inputSource) {
    
    // G.726使用动态负载类型
    const char *mimeType = "G726-32"; // 默认32kbps
    unsigned char payloadFormatCode = rtpPayloadTypeIfDynamic;
    
    // 根据位深度设置正确的MIME类型
    switch(fBitsPerSample) {
        case 2:
            mimeType = "G726-16";
            break;
        case 3:
            mimeType = "G726-24";
            break;
        case 4:
            mimeType = "G726-32";
            break;
        case 5:
            mimeType = "G726-40";
            break;
        default:
            mimeType = "G726-32";
            break;
    }

    return SimpleRTPSink::createNew(envir(), rtpGroupsock,
            payloadFormatCode, fSamplingFrequency,
            "audio", mimeType, fNumChannels);
}
