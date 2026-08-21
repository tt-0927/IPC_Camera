
#ifndef _CAPABILITYINFOCONVERT_H
#define _CAPABILITYINFOCONVERT_H

#include <string>
#include <vector>
#include <set>

#include "Json.h"

// 库通用头文件
#ifdef NET_TV_SDK_SERVER_API
    #include "NetTVSDKServerInterface.h"
#elif defined(NET_TV_SDK_CLIENT_API)
    #include "NetTVSDKClientInterface.h"
#else
    #include "NetTVSDKCommon.h"
#endif

namespace SDKConvert
{
    /**
     * @brief 视频分辨率结构体转换
     */
    void deal(Json::Object* pRootJson, NET_TV_VIDEO_RESOLUTION_S& stInfo, bool bOutStruct);

    /**
     * @brief 取值范围结构体转换
     */
    void deal(Json::Object* pRootJson, NET_TV_RANGE_S& stInfo, bool bOutStruct);

    /**
     * @brief 视频编码参数配置转换
     */
    void deal(Json::Object* pRootJson, NET_TV_VIDEO_ENCODE_OPTION_S& stInfo, bool bOutStruct);

    /**
     * @brief 视频编码格式能力转换
     */
    void deal(Json::Object* pRootJson, NET_TV_VIDEO_ENCODE_ABILITY_S& stInfo, bool bOutStruct);

    /**
     * @brief 视频码流参数能力集转换 (单个码流)
     */
    void deal(Json::Object* pRootJson, NET_TV_VIDEO_STREAM_CAP_S& stInfo, bool bOutStruct);

    /**
     * @brief 视频编码能力集转换 (多码流, NET_TV_CAP_VIDEO_ENCODE)
     */
    void deal(Json::Object* pRootJson, NET_TV_VIDEO_ENCODE_CAP_S& stInfo, bool bOutStruct);

    /**
     * @brief 音频范围转换
     */
    void deal(Json::Object* pRootJson, NET_TV_AUDIO_RANGE_S& stInfo, bool bOutStruct);

    /**
     * @brief 音频格式能力转换
     */
    void deal(Json::Object* pRootJson, NET_TV_AUDIO_FORMAT_CAP_S& stInfo, bool bOutStruct);

    /**
     * @brief 音频编码能力集转换 (多码流, NET_TV_CAP_VIDEO_ENCODE)
     */
    void deal(Json::Object* pRootJson, NET_TV_AUDIO_CAP_S& stInfo, bool bOutStruct);

    /**
     * @brief OSD参数能力集转换 (NET_TV_CAP_OSD)
     */
    void deal(Json::Object* pRootJson, NET_TV_OSD_CAP_S& stInfo, bool bOutStruct);

    // ==================== 后续扩展能力集 ====================
    // NET_TV_CAP_OSD: void deal(Json::Object*, NET_TV_OSD_CAP_S&, bool);
    // NET_TV_CAP_SMART: void deal(Json::Object*, NET_TV_SMART_CAP_S&, bool);
    // NET_TV_CAP_IMAGE: void deal(Json::Object*, NET_TV_IMAGE_CAP_S&, bool);
    // NET_TV_CAP_AUDIO: void deal(Json::Object*, NET_TV_AUDIO_CAP_S&, bool);
};

#endif
