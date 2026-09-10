/**
 * @FilePath     : plugin_version_utils.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-22 11:04:22
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-22 11:34:01
 * @Description  : 板端网页插件版本读取工具实现
 */

#include <cstddef>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>

#include "plugin_version_utils.h"

#include "dlog.h"
#include "path_define.h"

namespace PluginVersionUtils_NS
{
namespace
{
/* S81appinit 创建的固定软链接，始终指向当前实际启用的插件文件。 */
constexpr char PLUGIN_LINK_PATH[] = THIRD_PATRY_PATH "IpcComponents.exe";
/* 插件文件名的固定前缀，用于阻止无关文件被误识别为版本文件。 */
constexpr char PLUGIN_FILE_PREFIX[] = "IpcComponents-V";
/* 插件安装包的固定扩展名，用于限定可解析的目标文件。 */
constexpr char PLUGIN_FILE_SUFFIX[] = ".exe";
/* 设备信息 PluginVersion 的固定前缀，必须与既有版本字段契约保持一致。 */
constexpr char DEVICE_INFO_VERSION_PREFIX[] = "V";
/* 板端无法确定插件版本时的明确降级值，禁止伪造编译期版本。 */
constexpr char UNKNOWN_PLUGIN_VERSION[] = "Unknown";

/**
 * @brief   : 从网页插件文件名中提取版本字符串
 * @param    {const std::string &} strFileName：软链接目标的文件名
 * @return  {std::string} 合法时返回不带 V 前缀的版本；文件名格式非法时返回空字符串
 * @note    : 文件名由 S81appinit 按版本排序后选定，此处只校验命名契约，不重复目录扫描与版本裁决。
 */
std::string extract_version_from_file_name(const std::string &strFileName)
{
    const std::string strPrefix = PLUGIN_FILE_PREFIX;
    const std::string strSuffix = PLUGIN_FILE_SUFFIX;
    if (strFileName.size() <= strPrefix.size() + strSuffix.size() || strFileName.compare(0, strPrefix.size(), strPrefix) != 0 ||
        strFileName.compare(strFileName.size() - strSuffix.size(), strSuffix.size(), strSuffix) != 0)
    {
        return std::string();
    }

    return strFileName.substr(strPrefix.size(), strFileName.size() - strPrefix.size() - strSuffix.size());
}
} // namespace

std::string get_active_version()
{
    char achLinkTarget[PATH_MAX] = { 0 };
    const ssize_t nLinkLength = readlink(PLUGIN_LINK_PATH, achLinkTarget, sizeof(achLinkTarget));
    if (nLinkLength <= 0 || nLinkLength >= static_cast<ssize_t>(sizeof(achLinkTarget)))
    {
        dlog_warn("读取网页插件软链接失败 [%s]", PLUGIN_LINK_PATH);
        return UNKNOWN_PLUGIN_VERSION;
    }

    /* readlink 不会追加字符串结束符，必须在已校验长度后手动补齐以保证日志输出安全。 */
    achLinkTarget[nLinkLength] = '\0';

    struct stat stPluginFile = {};
    if (stat(PLUGIN_LINK_PATH, &stPluginFile) != 0 || !S_ISREG(stPluginFile.st_mode))
    {
        dlog_warn("网页插件软链接目标无效 [%s]->[%s]", PLUGIN_LINK_PATH, achLinkTarget);
        return UNKNOWN_PLUGIN_VERSION;
    }

    const std::string strLinkTarget(achLinkTarget, static_cast<std::size_t>(nLinkLength));
    const std::size_t nFileNamePos = strLinkTarget.find_last_of('/');
    const std::string strFileName = strLinkTarget.substr(nFileNamePos == std::string::npos ? 0 : nFileNamePos + 1);
    const std::string strVersion = extract_version_from_file_name(strFileName);
    if (strVersion.empty())
    {
        dlog_warn("网页插件文件名不符合版本命名规范 [%s]", strFileName.c_str());
        return UNKNOWN_PLUGIN_VERSION;
    }

    return std::string(DEVICE_INFO_VERSION_PREFIX) + strVersion;
}
} // namespace PluginVersionUtils_NS
