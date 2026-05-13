/**
 * @FilePath     : face_http_push_config.hpp
 * @Description  : Face event HTTP push configuration
 */

#pragma once

#include <string>

namespace FaceDetectInternal
{
/**
 * @brief   : 人脸 HTTP 推送配置
 * @note    : 支持统一 URL，也支持抓拍、比对分别配置 URL
 */
struct FaceHttpPushConfig_S
{
    /* 是否启用 HTTP 推送 */
    bool bEnable = false;
    /* 默认推送地址，抓拍/比对地址为空时使用该地址 */
    std::string strUrl;
    /* 人脸抓拍推送地址 */
    std::string strCaptureUrl;
    /* 人脸比对推送地址 */
    std::string strCompareUrl;
    /* 平台鉴权 Token，非空时通过 Authorization: Bearer 传递 */
    std::string strToken;
};

/**
 * @brief   : 人脸 HTTP 推送配置管理器
 */
class CFaceHttpPushConfig
{
public:
    /**
     * @brief   : 加载 HTTP 推送配置
     * @return   {FaceHttpPushConfig_S} HTTP 推送配置
     * @note    : 优先读取配置文件 /opt/cam/.config/user_data/face_http_push.json，再使用环境变量覆盖
     */
    static FaceHttpPushConfig_S load();

    /**
     * @brief   : 判断人脸抓拍 HTTP 推送是否可用
     * @param    {FaceHttpPushConfig_S} &stConfig：HTTP 推送配置
     * @return   {bool} true：可推送 false：不可推送
     */
    static bool isCaptureEnabled(const FaceHttpPushConfig_S &stConfig);

    /**
     * @brief   : 判断人脸比对 HTTP 推送是否可用
     * @param    {FaceHttpPushConfig_S} &stConfig：HTTP 推送配置
     * @return   {bool} true：可推送 false：不可推送
     */
    static bool isCompareEnabled(const FaceHttpPushConfig_S &stConfig);
};
} // namespace FaceDetectInternal
