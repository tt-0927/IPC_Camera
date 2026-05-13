/**
 * @FilePath     : face_http_push_config.cpp
 * @Description  : Face event HTTP push configuration implementation
 */

#include "face_http_push_config.hpp"

#include <cstdlib>
#include <fstream>
#include <sstream>

#include "Json.h"
#include "path_define.h"

namespace
{
constexpr const char *FACE_HTTP_PUSH_CONFIG_FILE = USER_DATA_PATH "face_http_push.json";

/**
 * @brief   : 获取非空环境变量
 * @param    {char} *pName：环境变量名称
 * @return   {char}* 环境变量值，未配置或为空时返回 nullptr
 */
const char *get_env(const char *pName)
{
    const char *pValue = std::getenv(pName);
    return (pValue == nullptr || pValue[0] == '\0') ? nullptr : pValue;
}

/**
 * @brief   : 将字符串解析为布尔值
 * @param    {std::string} &strValue：布尔字符串
 * @return   {bool} true：启用 false：关闭
 * @note    : 支持 1/true/on/yes 等常见启用写法
 */
bool parse_bool(const std::string &strValue)
{
    return strValue == "1" ||
           strValue == "true" ||
           strValue == "TRUE" ||
           strValue == "on" ||
           strValue == "ON" ||
           strValue == "yes" ||
           strValue == "YES";
}

/**
 * @brief   : 读取文本文件内容
 * @param    {std::string} &strPath：文件路径
 * @return   {std::string} 文件内容，读取失败返回空字符串
 */
std::string read_file(const std::string &strPath)
{
    std::ifstream file(strPath);
    if (!file.is_open())
    {
        return std::string();
    }

    std::ostringstream oss;
    oss << file.rdbuf();
    return oss.str();
}

/**
 * @brief   : 从配置文件加载 HTTP 推送参数
 * @param    {FaceHttpPushConfig_S} &stConfig：输出配置
 * @return   {void}
 */
void load_file_config(FaceDetectInternal::FaceHttpPushConfig_S &stConfig)
{
    const std::string strContent = read_file(FACE_HTTP_PUSH_CONFIG_FILE);
    if (strContent.empty())
    {
        return;
    }

    Json::Object *pRoot = Json::init(strContent);
    if (pRoot == nullptr)
    {
        return;
    }

    Json::get(pRoot, "Enable", stConfig.bEnable);
    Json::get(pRoot, "Url", stConfig.strUrl);
    Json::get(pRoot, "CaptureUrl", stConfig.strCaptureUrl);
    Json::get(pRoot, "CompareUrl", stConfig.strCompareUrl);
    Json::get(pRoot, "Token", stConfig.strToken);
    Json::deinit(pRoot);
}

/**
 * @brief   : 使用环境变量覆盖 HTTP 推送参数
 * @param    {FaceHttpPushConfig_S} &stConfig：待覆盖配置
 * @return   {void}
 */
void apply_env_config(FaceDetectInternal::FaceHttpPushConfig_S &stConfig)
{
    if (const char *pValue = get_env("FACE_HTTP_PUSH_ENABLE"))
    {
        stConfig.bEnable = parse_bool(pValue);
    }
    if (const char *pValue = get_env("FACE_HTTP_PUSH_URL"))
    {
        stConfig.strUrl = pValue;
    }
    if (const char *pValue = get_env("FACE_HTTP_CAPTURE_URL"))
    {
        stConfig.strCaptureUrl = pValue;
    }
    if (const char *pValue = get_env("FACE_HTTP_COMPARE_URL"))
    {
        stConfig.strCompareUrl = pValue;
    }
    if (const char *pValue = get_env("FACE_HTTP_PUSH_TOKEN"))
    {
        stConfig.strToken = pValue;
    }
}
} // namespace

namespace FaceDetectInternal
{
/**
 * @brief   : 加载 HTTP 推送配置
 * @return   {FaceHttpPushConfig_S} HTTP 推送配置
 */
FaceHttpPushConfig_S CFaceHttpPushConfig::load()
{
    FaceHttpPushConfig_S stConfig;
    load_file_config(stConfig);
    apply_env_config(stConfig);

    if (stConfig.strCaptureUrl.empty())
    {
        stConfig.strCaptureUrl = stConfig.strUrl;
    }
    if (stConfig.strCompareUrl.empty())
    {
        stConfig.strCompareUrl = stConfig.strUrl;
    }
    return stConfig;
}

/**
 * @brief   : 判断人脸抓拍 HTTP 推送是否可用
 * @param    {FaceHttpPushConfig_S} &stConfig：HTTP 推送配置
 * @return   {bool} true：可推送 false：不可推送
 */
bool CFaceHttpPushConfig::isCaptureEnabled(const FaceHttpPushConfig_S &stConfig)
{
    return stConfig.bEnable && !stConfig.strCaptureUrl.empty();
}

/**
 * @brief   : 判断人脸比对 HTTP 推送是否可用
 * @param    {FaceHttpPushConfig_S} &stConfig：HTTP 推送配置
 * @return   {bool} true：可推送 false：不可推送
 */
bool CFaceHttpPushConfig::isCompareEnabled(const FaceHttpPushConfig_S &stConfig)
{
    return stConfig.bEnable && !stConfig.strCompareUrl.empty();
}
} // namespace FaceDetectInternal
