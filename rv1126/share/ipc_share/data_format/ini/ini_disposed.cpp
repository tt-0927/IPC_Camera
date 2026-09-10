/**
 * @FilePath     : ini_disposed.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-02-27 13:40:43
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-27 20:29:06
 * @Description  : INI配置文件读写接口与只读解析器实现
 */

#include "ini_disposed.h"

#include "IpcRet.h"
#include "dlog.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cerrno>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <utility>
#include <vector>

namespace
{
/* 单个INI文件允许的最大长度，防止异常配置耗尽进程内存。 */
constexpr std::streamoff MAX_INI_FILE_SIZE = 1024 * 1024;

/**
 * @brief   : 删除字符串首尾空白
 * @param    {const std::string &} strValue：原始字符串
 * @return   {std::string} 去除首尾空白后的字符串
 */
std::string trim(const std::string &strValue)
{
    auto is_space = [](unsigned char chValue)
    {
        return std::isspace(chValue) != 0;
    };
    auto stBegin = std::find_if_not(strValue.begin(), strValue.end(), is_space);
    if (stBegin == strValue.end())
    {
        return std::string();
    }

    auto stEnd = std::find_if_not(strValue.rbegin(), strValue.rend(), is_space).base();
    return std::string(stBegin, stEnd);
}

/**
 * @brief   : 将ASCII字符串转换为小写
 * @param    {const std::string &} strValue：原始字符串
 * @return   {std::string} 小写字符串
 */
std::string to_lower(const std::string &strValue)
{
    std::string strResult = strValue;
    std::transform(strResult.begin(),
                   strResult.end(),
                   strResult.begin(),
                   [](unsigned char chValue)
                   {
                       return static_cast<char>(std::tolower(chValue));
                   });
    return strResult;
}

/**
 * @brief   : 删除未被引号包围的行尾注释
 * @param    {const std::string &} strValue：待处理字符串
 * @return   {std::string} 删除注释后的字符串
 * @note    : `;`或`#`位于行首或前一字符为空白时才作为注释，避免破坏URL和Token。
 */
std::string strip_inline_comment(const std::string &strValue)
{
    char chQuote = '\0';
    bool bEscaped = false;
    for (size_t nIndex = 0; nIndex < strValue.size(); ++nIndex)
    {
        const char chValue = strValue[nIndex];
        if (bEscaped)
        {
            bEscaped = false;
            continue;
        }
        if (chValue == '\\')
        {
            bEscaped = true;
            continue;
        }
        if (chQuote != '\0')
        {
            if (chValue == chQuote)
            {
                chQuote = '\0';
            }
            continue;
        }
        if (chValue == '\'' || chValue == '"')
        {
            chQuote = chValue;
            continue;
        }
        if ((chValue == ';' || chValue == '#') &&
            (nIndex == 0 || std::isspace(static_cast<unsigned char>(strValue[nIndex - 1])) != 0))
        {
            return strValue.substr(0, nIndex);
        }
    }
    return strValue;
}

/**
 * @brief   : 删除包围整个值的单双引号
 * @param    {const std::string &} strValue：已去除空白和注释的值
 * @return   {std::string} 规范化值
 */
std::string unquote(const std::string &strValue)
{
    if (strValue.size() < 2)
    {
        return strValue;
    }
    const char chFirst = strValue.front();
    const char chLast = strValue.back();
    if ((chFirst == '"' && chLast == '"') || (chFirst == '\'' && chLast == '\''))
    {
        return strValue.substr(1, strValue.size() - 2);
    }
    return strValue;
}

/**
 * @brief   : 判断逻辑行是否使用反斜杠续行
 * @param    {const std::string &} strLine：物理行
 * @return   {bool} true：需要续行，false：当前逻辑行结束
 */
bool has_continuation(const std::string &strLine)
{
    const std::string strTrimmed = trim(strLine);
    if (strTrimmed.empty() || strTrimmed.back() != '\\')
    {
        return false;
    }

    size_t nSlashCount = 0;
    for (auto stIt = strTrimmed.rbegin(); stIt != strTrimmed.rend() && *stIt == '\\'; ++stIt)
    {
        ++nSlashCount;
    }
    return (nSlashCount % 2) == 1;
}

/**
 * @brief   : 安全复制C字符串并保证结尾为零
 * @param    {char *} pDst：目标缓冲区
 * @param    {int} nDstSize：目标缓冲区长度
 * @param    {const std::string &} strValue：待复制字符串
 * @return   {void}
 */
void copy_to_buffer(char *pDst, int nDstSize, const std::string &strValue)
{
    if (pDst == nullptr || nDstSize <= 0)
    {
        return;
    }
    const size_t nCopySize = std::min(static_cast<size_t>(nDstSize - 1), strValue.size());
    if (nCopySize > 0)
    {
        std::copy_n(strValue.data(), nCopySize, pDst);
    }
    pDst[nCopySize] = '\0';
}

/**
 * @brief   : 从配置行提取段名称
 * @param    {const std::string &} strLine：配置行
 * @param    {std::string &} strSection：输出段名称
 * @return   {bool} true：是合法段行，false：不是段行
 */
bool parse_section_line(const std::string &strLine, std::string &strSection)
{
    const std::string strClean = trim(strip_inline_comment(strLine));
    if (strClean.size() < 2 || strClean.front() != '[' || strClean.back() != ']')
    {
        return false;
    }
    strSection = trim(strClean.substr(1, strClean.size() - 2));
    return !strSection.empty();
}

/**
 * @brief   : 从配置行提取键值
 * @param    {const std::string &} strLine：配置行
 * @param    {std::string &} strKey：输出键名
 * @param    {std::string &} strValue：输出值
 * @return   {bool} true：是合法键值行，false：不是键值行
 */
bool parse_key_value_line(const std::string &strLine, std::string &strKey, std::string &strValue)
{
    const std::string strClean = trim(strip_inline_comment(strLine));
    if (strClean.empty() || strClean.front() == ';' || strClean.front() == '#')
    {
        return false;
    }

    char chQuote = '\0';
    bool bEscaped = false;
    size_t nEqualIndex = std::string::npos;
    for (size_t nIndex = 0; nIndex < strClean.size(); ++nIndex)
    {
        const char chValue = strClean[nIndex];
        if (bEscaped)
        {
            bEscaped = false;
            continue;
        }
        if (chValue == '\\')
        {
            bEscaped = true;
            continue;
        }
        if (chQuote != '\0')
        {
            if (chValue == chQuote)
            {
                chQuote = '\0';
            }
            continue;
        }
        if (chValue == '\'' || chValue == '"')
        {
            chQuote = chValue;
            continue;
        }
        if (chValue == '=')
        {
            nEqualIndex = nIndex;
            break;
        }
    }
    if (nEqualIndex == std::string::npos)
    {
        return false;
    }

    strKey = trim(strClean.substr(0, nEqualIndex));
    strValue = unquote(trim(strClean.substr(nEqualIndex + 1)));
    return !strKey.empty();
}

/**
 * @brief   : 严格解析整数
 * @param    {const std::string &} strValue：数字字符串
 * @param    {int &} nValue：输出整数
 * @return   {bool} true：解析成功，false：格式或范围错误
 */
bool parse_integer(const std::string &strValue, int &nValue)
{
    if (strValue.empty())
    {
        return false;
    }

    errno = 0;
    char *pEnd = nullptr;
    const long long llParsed = std::strtoll(strValue.c_str(), &pEnd, 10);
    if (errno == ERANGE || pEnd == strValue.c_str() || pEnd == nullptr || *pEnd != '\0' ||
        llParsed < std::numeric_limits<int>::min() || llParsed > std::numeric_limits<int>::max())
    {
        return false;
    }
    nValue = static_cast<int>(llParsed);
    return true;
}

/**
 * @brief   : 严格解析无符号64位整数
 * @param    {const std::string &} strValue：数字字符串
 * @param    {uint64_t &} u64Value：输出整数
 * @return   {bool} true：解析成功，false：格式或范围错误
 */
bool parse_integer(const std::string &strValue, uint64_t &u64Value)
{
    if (strValue.empty() || strValue.front() == '-')
    {
        return false;
    }

    errno = 0;
    char *pEnd = nullptr;
    const unsigned long long ullParsed = std::strtoull(strValue.c_str(), &pEnd, 10);
    if (errno == ERANGE || pEnd == strValue.c_str() || pEnd == nullptr || *pEnd != '\0')
    {
        return false;
    }
    u64Value = static_cast<uint64_t>(ullParsed);
    return true;
}

/**
 * @brief   : 读取文本文件所有行
 * @param    {const std::string &} strFilePath：文件路径
 * @param    {std::vector<std::string> &} vecLines：输出行集合
 * @return   {int} OK：成功，ERR_OPEN/ERR_FREAD：失败
 */
int read_all_lines(const std::string &strFilePath, std::vector<std::string> &vecLines)
{
    std::ifstream stFile(strFilePath);
    if (!stFile.is_open())
    {
        return ERR_OPEN;
    }

    stFile.seekg(0, std::ios::end);
    const std::streamoff nFileSize = stFile.tellg();
    if (nFileSize < 0 || nFileSize > MAX_INI_FILE_SIZE)
    {
        return ERR_FILE_ERR;
    }
    stFile.seekg(0, std::ios::beg);

    std::string strLine;
    while (std::getline(stFile, strLine))
    {
        if (!strLine.empty() && strLine.back() == '\r')
        {
            strLine.pop_back();
        }
        vecLines.push_back(strLine);
    }
    if (stFile.bad())
    {
        return ERR_FREAD;
    }
    return OK;
}

/**
 * @brief   : 说明INI文件读取失败原因
 * @param    {int} nRet：读取阶段返回码
 * @return   {const char *} 用于日志输出的失败原因
 * @note    : 此处只覆盖read_all_lines的返回码，其他错误保留通用描述，避免返回码变化后误导排障。
 */
const char *describe_read_error(int nRet)
{
    switch (nRet)
    {
    case ERR_OPEN:
        return "INI文件无法打开";
    case ERR_FILE_ERR:
        return "INI文件大小获取失败或超过1MB限制";
    case ERR_FREAD:
        return "INI文件读取失败";
    default:
        return "INI文件读取异常";
    }
}

/**
 * @brief   : 原子更新单个INI键值
 * @param    {const std::string &} strSection：配置段名称
 * @param    {const std::string &} strKey：配置项名称
 * @param    {const std::string &} strValue：配置值
 * @param    {const std::string &} strFilePath：INI文件路径
 * @return   {int} OK：成功，非OK：失败
 */
int update_ini_value(const std::string &strSection,
                     const std::string &strKey,
                     const std::string &strValue,
                     const std::string &strFilePath)
{
    if (strSection.empty() || strKey.empty() || strFilePath.empty())
    {
        return ERR_PARAM;
    }

    std::vector<std::string> vecLines;
    struct stat stFileInfo = {};
    const int nStatRet = stat(strFilePath.c_str(), &stFileInfo);
    if (nStatRet != 0 && errno != ENOENT)
    {
        return ERR_OPEN;
    }
    const bool bFileExists = (nStatRet == 0);
    if (bFileExists)
    {
        if (S_ISDIR(stFileInfo.st_mode))
        {
            return ERR_FILE_ERR;
        }
        const int nReadRet = read_all_lines(strFilePath, vecLines);
        if (nReadRet != OK)
        {
            return nReadRet;
        }
    }

    bool bSectionFound = false;
    bool bKeyUpdated = false;
    size_t nInsertIndex = vecLines.size();
    std::string strCurrentSection;
    for (size_t nIndex = 0; nIndex < vecLines.size(); ++nIndex)
    {
        std::string strParsedSection;
        if (parse_section_line(vecLines[nIndex], strParsedSection))
        {
            if (strCurrentSection == strSection && !bKeyUpdated)
            {
                nInsertIndex = nIndex;
            }
            strCurrentSection = strParsedSection;
            if (strCurrentSection == strSection)
            {
                bSectionFound = true;
                nInsertIndex = nIndex + 1;
            }
            continue;
        }

        if (strCurrentSection != strSection)
        {
            continue;
        }
        std::string strParsedKey;
        std::string strParsedValue;
        if (parse_key_value_line(vecLines[nIndex], strParsedKey, strParsedValue))
        {
            nInsertIndex = nIndex + 1;
            if (strParsedKey == strKey)
            {
                vecLines[nIndex] = strKey + "=" + strValue;
                bKeyUpdated = true;
                break;
            }
        }
    }

    if (!bKeyUpdated)
    {
        if (bSectionFound)
        {
            vecLines.insert(vecLines.begin() + static_cast<std::ptrdiff_t>(nInsertIndex), strKey + "=" + strValue);
        }
        else
        {
            if (!vecLines.empty() && !vecLines.back().empty())
            {
                vecLines.emplace_back();
            }
            vecLines.push_back("[" + strSection + "]");
            vecLines.push_back(strKey + "=" + strValue);
        }
    }

    const std::string strTempPath = strFilePath + ".tmp";
    std::ofstream stOutput(strTempPath, std::ios::trunc);
    if (!stOutput.is_open())
    {
        return ERR_OPEN;
    }
    for (const std::string &strLine : vecLines)
    {
        stOutput << strLine << '\n';
    }
    stOutput.flush();
    if (!stOutput.good())
    {
        stOutput.close();
        std::remove(strTempPath.c_str());
        return ERR_FWRITE;
    }
    stOutput.close();

    if (bFileExists)
    {
        /* 保留原文件权限，避免原子替换后改变配置访问策略。 */
        if (chmod(strTempPath.c_str(), stFileInfo.st_mode) != 0)
        {
            std::remove(strTempPath.c_str());
            return ERR_FWRITE;
        }
    }
    if (std::rename(strTempPath.c_str(), strFilePath.c_str()) != 0)
    {
        std::remove(strTempPath.c_str());
        return ERR_FWRITE;
    }
    return OK;
}
} // namespace

CIniReader::CIniReader(const std::string &strFilePath)
{
    /* 构造函数不能返回错误码，加载结果保存在is_loaded和last_error中。 */
    static_cast<void>(load(strFilePath));
}

int CIniReader::load(const std::string &strFilePath, IniDuplicatePolicy_E enDuplicatePolicy)
{
    clear();
    if (strFilePath.empty())
    {
        return set_load_error(ERR_PARAM, strFilePath, "INI文件路径为空");
    }

    std::vector<std::string> vecLines;
    int nRet = read_all_lines(strFilePath, vecLines);
    if (nRet != OK)
    {
        return set_load_error(nRet, strFilePath, describe_read_error(nRet));
    }

    SectionMap_T mapParsedSections;
    std::string strCurrentSection;
    std::string strLogicalLine;
    size_t nLogicalStartLine = 0;
    /* 兼容模式遇到重复段后，忽略该段全部内容，保持历史“首个同名段生效”的行为。 */
    bool bIgnoreCurrentSection = false;
    for (size_t nIndex = 0; nIndex < vecLines.size(); ++nIndex)
    {
        std::string strPhysicalLine = vecLines[nIndex];
        if (strLogicalLine.empty())
        {
            nLogicalStartLine = nIndex + 1;
        }
        if (has_continuation(strPhysicalLine))
        {
            std::string strPart = trim(strPhysicalLine);
            strPart.pop_back();
            strLogicalLine += trim(strPart);
            strLogicalLine.push_back(' ');
            continue;
        }
        strLogicalLine += strPhysicalLine;

        const std::string strCleanLine = trim(strip_inline_comment(strLogicalLine));
        strLogicalLine.clear();
        if (strCleanLine.empty())
        {
            continue;
        }

        std::string strSection;
        if (parse_section_line(strCleanLine, strSection))
        {
            strCurrentSection = strSection;
            const auto stSectionIt = mapParsedSections.find(strCurrentSection);
            if (stSectionIt != mapParsedSections.end())
            {
                if (enDuplicatePolicy == IniDuplicatePolicy_E::REJECT)
                {
                    std::ostringstream stError;
                    stError << "INI配置段重复, 文件:" << strFilePath << ", 行:" << nLogicalStartLine
                            << ", 段:" << strCurrentSection;
                    return set_load_error(ERR_PARSE, strFilePath, stError.str());
                }
                bIgnoreCurrentSection = true;
                continue;
            }

            mapParsedSections.emplace(strCurrentSection, KeyValueMap_T{});
            bIgnoreCurrentSection = false;
            continue;
        }

        std::string strKey;
        std::string strValue;
        if (!parse_key_value_line(strCleanLine, strKey, strValue))
        {
            std::ostringstream stError;
            stError << "INI格式错误, 文件:" << strFilePath << ", 行:" << nLogicalStartLine;
            return set_load_error(ERR_PARSE, strFilePath, stError.str());
        }
        if (strCurrentSection.empty())
        {
            std::ostringstream stError;
            stError << "INI键值缺少配置段, 文件:" << strFilePath << ", 行:" << nLogicalStartLine;
            return set_load_error(ERR_PARSE, strFilePath, stError.str());
        }
        if (bIgnoreCurrentSection)
        {
            continue;
        }

        const auto stInsertResult = mapParsedSections[strCurrentSection].emplace(strKey, strValue);
        if (!stInsertResult.second && enDuplicatePolicy == IniDuplicatePolicy_E::REJECT)
        {
            std::ostringstream stError;
            stError << "INI配置项重复, 文件:" << strFilePath << ", 行:" << nLogicalStartLine << ", 段:" << strCurrentSection
                    << ", 键:" << strKey;
            return set_load_error(ERR_PARSE, strFilePath, stError.str());
        }
    }
    if (!strLogicalLine.empty())
    {
        return set_load_error(ERR_PARSE, strFilePath, "INI文件以未完成的续行结束: " + strFilePath);
    }

    m_mapSections = std::move(mapParsedSections);
    m_strFilePath = strFilePath;
    m_bLoaded = true;
    return OK;
}

void CIniReader::clear()
{
    m_mapSections.clear();
    m_strFilePath.clear();
    m_strLastError.clear();
    m_bLoaded = false;
}

bool CIniReader::is_loaded() const
{
    return m_bLoaded;
}

bool CIniReader::has_section(const std::string &strSection) const
{
    return m_mapSections.find(strSection) != m_mapSections.end();
}

bool CIniReader::has_key(const std::string &strSection, const std::string &strKey) const
{
    const auto stSection = m_mapSections.find(strSection);
    return stSection != m_mapSections.end() && stSection->second.find(strKey) != stSection->second.end();
}

int CIniReader::get_string(const std::string &strSection, const std::string &strKey, std::string &strValue) const
{
    if (!m_bLoaded)
    {
        return set_error(ERR_UNINIT, "INI文件尚未加载");
    }
    const auto stSection = m_mapSections.find(strSection);
    if (stSection == m_mapSections.end())
    {
        return set_error(ERR_NOT_EXIST, "INI配置段不存在: " + strSection);
    }
    const auto stValue = stSection->second.find(strKey);
    if (stValue == stSection->second.end())
    {
        return set_error(ERR_NOT_EXIST, "INI配置项不存在: " + strSection + "." + strKey);
    }
    strValue = stValue->second;
    m_strLastError.clear();
    return OK;
}

int CIniReader::get_int(const std::string &strSection, const std::string &strKey, int &nValue) const
{
    std::string strValue;
    int nRet = get_string(strSection, strKey, strValue);
    if (nRet != OK)
    {
        return nRet;
    }
    int nParsed = 0;
    if (!parse_integer(strValue, nParsed))
    {
        return set_error(ERR_PARSE, "INI整数格式错误: " + strSection + "." + strKey + "=" + strValue);
    }
    nValue = nParsed;
    return OK;
}

int CIniReader::get_uint64(const std::string &strSection, const std::string &strKey, uint64_t &u64Value) const
{
    std::string strValue;
    int nRet = get_string(strSection, strKey, strValue);
    if (nRet != OK)
    {
        return nRet;
    }
    uint64_t u64Parsed = 0;
    if (!parse_integer(strValue, u64Parsed))
    {
        return set_error(ERR_PARSE, "INI无符号整数格式错误: " + strSection + "." + strKey + "=" + strValue);
    }
    u64Value = u64Parsed;
    return OK;
}

int CIniReader::get_bool(const std::string &strSection, const std::string &strKey, bool &bValue) const
{
    std::string strValue;
    int nRet = get_string(strSection, strKey, strValue);
    if (nRet != OK)
    {
        return nRet;
    }

    const std::string strLowerValue = to_lower(trim(strValue));
    if (strLowerValue == "true" || strLowerValue == "yes" || strLowerValue == "on" || strLowerValue == "1")
    {
        bValue = true;
        return OK;
    }
    if (strLowerValue == "false" || strLowerValue == "no" || strLowerValue == "off" || strLowerValue == "0")
    {
        bValue = false;
        return OK;
    }
    return set_error(ERR_PARSE, "INI布尔格式错误: " + strSection + "." + strKey + "=" + strValue);
}

const std::string &CIniReader::last_error() const
{
    return m_strLastError;
}

int CIniReader::set_error(int nRet, const std::string &strError) const
{
    m_strLastError = strError;
    return nRet;
}

int CIniReader::set_load_error(int nRet, const std::string &strFilePath, const std::string &strError) const
{
    dlog_error("INI加载失败, 文件:%s, ret:%d, 原因:%s", strFilePath.c_str(), nRet, strError.c_str());
    return set_error(nRet, strError);
}

extern "C"
{
    int ini_read_profile_char(const char *section,
                              const char *key,
                              char *value,
                              int size,
                              const char *default_value,
                              const char *file)
    {
        if (section == nullptr || key == nullptr || value == nullptr || size <= 0 || file == nullptr)
        {
            return REDA_INI_FAIL;
        }

        CIniReader stReader;
        std::string strValue;
        if (stReader.load(file, IniDuplicatePolicy_E::KEEP_FIRST) != OK || stReader.get_string(section, key, strValue) != OK)
        {
            copy_to_buffer(value, size, default_value == nullptr ? std::string() : std::string(default_value));
            return REDA_INI_FAIL;
        }

        copy_to_buffer(value, size, strValue);
        return REDA_INI_SUCCESS;
    }

    int ini_read_profile_int(const char *section, const char *key, int *nValue, int default_value, const char *file)
    {
        if (section == nullptr || key == nullptr || nValue == nullptr || file == nullptr)
        {
            return REDA_INI_FAIL;
        }

        CIniReader stReader;
        if (stReader.load(file, IniDuplicatePolicy_E::KEEP_FIRST) != OK || stReader.get_int(section, key, *nValue) != OK)
        {
            *nValue = default_value;
            return REDA_INI_FAIL;
        }
        return REDA_INI_SUCCESS;
    }

    int ini_write_profile_char(const char *section, const char *key, const char *value, const char *file)
    {
        if (section == nullptr || key == nullptr || value == nullptr || file == nullptr)
        {
            return REDA_INI_FAIL;
        }
        return update_ini_value(section, key, value, file) == OK ? REDA_INI_SUCCESS : REDA_INI_FAIL;
    }

    int ini_write_profile_int(const char *section, const char *key, int value, const char *file)
    {
        return ini_write_profile_char(section, key, std::to_string(value).c_str(), file);
    }
}

CIni::CIni(std::string strFileName) : m_strFileName(std::move(strFileName))
{
}

CIni::~CIni() = default;

void CIni::load(std::string strFileName)
{
    m_strFileName = std::move(strFileName);
}

int CIni::read(std::string strSection, std::string strKey, std::string &strValue, std::string strDefaultValue)
{
    CIniReader stReader;
    if (stReader.load(m_strFileName, IniDuplicatePolicy_E::KEEP_FIRST) != OK ||
        stReader.get_string(strSection, strKey, strValue) != OK)
    {
        strValue = strDefaultValue;
        return REDA_INI_FAIL;
    }
    return REDA_INI_SUCCESS;
}

int CIni::read(std::string strSection, std::string strKey, int &nValue, int nDefaultValue)
{
    CIniReader stReader;
    if (stReader.load(m_strFileName, IniDuplicatePolicy_E::KEEP_FIRST) != OK ||
        stReader.get_int(strSection, strKey, nValue) != OK)
    {
        nValue = nDefaultValue;
        return REDA_INI_FAIL;
    }
    return REDA_INI_SUCCESS;
}

int CIni::write(std::string strSection, std::string strKey, std::string strValue)
{
    return ini_write_profile_char(strSection.c_str(), strKey.c_str(), strValue.c_str(), m_strFileName.c_str());
}

int CIni::write(std::string strSection, std::string strKey, int nValue)
{
    return ini_write_profile_int(strSection.c_str(), strKey.c_str(), nValue, m_strFileName.c_str());
}
