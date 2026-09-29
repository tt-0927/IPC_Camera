/**
 * @FilePath     : codec_map.cpp
 * @Author       : zhouzr@kfb.cn
 * @Description  : 产品枚举 -> ipc_rtsp 库枚举映射实现（见 codec_map.h）
 */
#include "codec_map.h"

#include "rtsp_server.h"

namespace
{

/* 音频动态 PT：AAC/G.726 各占一个动态槽位。 */
constexpr uint8_t kAacPayloadType = 97;
constexpr uint8_t kG726PayloadType = 98;
/* G.711 使用 RFC 3551 静态 PT。 */
constexpr uint8_t kG711APayloadType = 8;
constexpr uint8_t kG711UPayloadType = 0;

} // namespace

namespace rtsp_smol
{

ipc_rtsp::Codec to_codec(Video_NS::VideoCodec_E codec)
{
    switch (codec)
    {
    case Video_NS::VideoCodec_E::H264:
        return ipc_rtsp::Codec::H264;
    case Video_NS::VideoCodec_E::H265:
        return ipc_rtsp::Codec::H265;
    case Video_NS::VideoCodec_E::MJPEG:
        return ipc_rtsp::Codec::MJPEG;
    default:
        return ipc_rtsp::Codec::Unknown;
    }
}

ipc_rtsp::AudioCodec to_audio_codec(Audio_NS::AudioFormat_E format)
{
    switch (format)
    {
    case Audio_NS::AudioFormat_E::AAC:
        return ipc_rtsp::AudioCodec::AAC;
    case Audio_NS::AudioFormat_E::G711A:
        return ipc_rtsp::AudioCodec::G711A;
    case Audio_NS::AudioFormat_E::G711U:
        return ipc_rtsp::AudioCodec::G711U;
    case Audio_NS::AudioFormat_E::G726:
        return ipc_rtsp::AudioCodec::G726_32;
    default:
        return ipc_rtsp::AudioCodec::None;
    }
}

uint8_t audio_payload_type(ipc_rtsp::AudioCodec codec)
{
    switch (codec)
    {
    case ipc_rtsp::AudioCodec::G711A:
        return kG711APayloadType;
    case ipc_rtsp::AudioCodec::G711U:
        return kG711UPayloadType;
    case ipc_rtsp::AudioCodec::AAC:
        return kAacPayloadType;
    case ipc_rtsp::AudioCodec::G726_32:
        return kG726PayloadType;
    default:
        return kAacPayloadType;
    }
}

ipc_rtsp::NalUnitType to_nal_type(Video_NS::VideoCodec_E codec, Video_NS::NalType_E type)
{
    if (codec == Video_NS::VideoCodec_E::H264)
    {
        switch (type)
        {
        case Video_NS::H264_TYPE_IDR:
            return ipc_rtsp::NalUnitType::IdrSlice;
        case Video_NS::H264_TYPE_SPS:
            return ipc_rtsp::NalUnitType::Sps;
        case Video_NS::H264_TYPE_PPS:
            return ipc_rtsp::NalUnitType::Pps;
        case Video_NS::H264_TYPE_SEI:
            return ipc_rtsp::NalUnitType::Sei;
        case Video_NS::H264_TYPE_AUD:
            return ipc_rtsp::NalUnitType::Aud;
        case Video_NS::H264_TYPE_SLICE:
        case Video_NS::H264_TYPE_DPA:
        case Video_NS::H264_TYPE_DPB:
        case Video_NS::H264_TYPE_DPC:
            return ipc_rtsp::NalUnitType::NonIdrSlice;
        default:
            return ipc_rtsp::NalUnitType::Other;
        }
    }

    if (codec == Video_NS::VideoCodec_E::H265)
    {
        switch (type)
        {
        case Video_NS::H265_TYPE_IDR_W_RADL:
        case Video_NS::H265_TYPE_IDR_N_LP:
            return ipc_rtsp::NalUnitType::IdrSlice;
        case Video_NS::H265_TYPE_CRA:
            return ipc_rtsp::NalUnitType::CraSlice;
        case Video_NS::H265_TYPE_VPS:
            return ipc_rtsp::NalUnitType::Vps;
        case Video_NS::H265_TYPE_SPS:
            return ipc_rtsp::NalUnitType::Sps;
        case Video_NS::H265_TYPE_PPS:
            return ipc_rtsp::NalUnitType::Pps;
        case Video_NS::H265_TYPE_AUD:
            return ipc_rtsp::NalUnitType::Aud;
        case Video_NS::H265_TYPE_SEI:
        case Video_NS::H265_TYPE_SEI_SUFFIX:
            return ipc_rtsp::NalUnitType::Sei;
        case Video_NS::H265_TYPE_TRAIL_N:
        case Video_NS::H265_TYPE_TRAIL_R:
        case Video_NS::H265_TYPE_RASL_N:
        case Video_NS::H265_TYPE_RASL_R:
        case Video_NS::H265_TYPE_RADL_N:
        case Video_NS::H265_TYPE_RADL_R:
            return ipc_rtsp::NalUnitType::NonIdrSlice;
        default:
            return ipc_rtsp::NalUnitType::Other;
        }
    }

    return ipc_rtsp::NalUnitType::Other;
}

} // namespace rtsp_smol
