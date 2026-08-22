/**
 * @file PlatformHttpClient.cpp
 * @author Codex
 * @date 2026-08-22
 * @brief Implements platform HTTP authentication, registration and image transfer.
 * @change 2026-08-22 Codex Initial implementation for platform migration.
 */

#include "PlatformHttpClient.h"

#include <cctype>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

#include <openssl/evp.h>

#include "NetSdkLog.h"
#include "cJSON.h"
#include "tvsdkhttplib.h"

namespace httplib = tvsdk::httplib;

#if defined(_WIN32)
#include <direct.h>
#include <sys/stat.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace
{
constexpr const char *PLATFORM_HTTP_LOGIN_PATH = "/api/auth/login";
constexpr const char *PLATFORM_HTTP_REGISTER_PATH = "/api/device/store_device";
constexpr const char *PLATFORM_HTTP_EVENT_IMAGE_PATH = "/api/device/upload_screen";

/**
 * @brief Copies a C string from an ABI field without reading past its bound.
 * @author Codex
 * @param [IN] pValue Source character array.
 * @param [IN] uCapacity Source array capacity.
 * @return Bounded string copy.
 */
static std::string CopyAbiString(const char *pValue, std::size_t uCapacity)
{
    if (pValue == nullptr || uCapacity == 0)
    {
        return std::string();
    }

    std::size_t uLength = 0;
    while (uLength < uCapacity && pValue[uLength] != '\0')
    {
        ++uLength;
    }
    return std::string(pValue, uLength);
}

/**
 * @brief Encodes text as standard Base64 without line breaks.
 * @author Codex
 * @param [IN] strInput Plain input bytes.
 * @return Base64 text or an empty string when input is empty or invalid.
 */
static std::string Base64Encode(const std::string &strInput)
{
    if (strInput.empty() || strInput.size() > static_cast<std::size_t>(INT_MAX))
    {
        return std::string();
    }

    const std::size_t uOutputSize = 4U * ((strInput.size() + 2U) / 3U);
    std::string strOutput(uOutputSize, '\0');
    const int nWritten = EVP_EncodeBlock(
        reinterpret_cast<unsigned char *>(&strOutput[0]),
        reinterpret_cast<const unsigned char *>(strInput.data()),
        static_cast<int>(strInput.size()));
    if (nWritten <= 0)
    {
        return std::string();
    }
    strOutput.resize(static_cast<std::size_t>(nWritten));
    return strOutput;
}

/**
 * @brief Percent-encodes one application/x-www-form-urlencoded value.
 * @author Codex
 * @param [IN] strValue Raw form value.
 * @return Encoded form value.
 */
static std::string EncodeFormValue(const std::string &strValue)
{
    std::ostringstream stOutput;
    stOutput << std::uppercase << std::hex;
    for (unsigned char byValue : strValue)
    {
        const bool bUnreserved =
            (byValue >= 'a' && byValue <= 'z') ||
            (byValue >= 'A' && byValue <= 'Z') ||
            (byValue >= '0' && byValue <= '9') ||
            byValue == '-' || byValue == '_' || byValue == '.' || byValue == '~';
        if (bUnreserved)
        {
            stOutput << static_cast<char>(byValue);
        }
        else
        {
            stOutput << '%' << std::setw(2) << std::setfill('0') << static_cast<int>(byValue);
        }
    }
    return stOutput.str();
}

/**
 * @brief Serializes and frees a cJSON object.
 * @author Codex
 * @param [IN] pRoot JSON root owned by the caller.
 * @return Compact JSON text.
 */
static std::string PrintJson(cJSON *pRoot)
{
    if (pRoot == nullptr)
    {
        return std::string();
    }

    char *pText = cJSON_PrintUnformatted(pRoot);
    std::string strText = pText != nullptr ? pText : std::string();
    if (pText != nullptr)
    {
        cJSON_free(pText);
    }
    return strText;
}

/**
 * @brief Reads a string field when present and type-correct.
 * @author Codex
 * @return Extracted string or an empty string.
 */
static std::string GetJsonString(cJSON *pRoot, const char *pName)
{
    cJSON *pItem = cJSON_GetObjectItemCaseSensitive(pRoot, pName);
    return cJSON_IsString(pItem) && pItem->valuestring != nullptr
               ? std::string(pItem->valuestring)
               : std::string();
}

/**
 * @brief Reads a file into a binary string.
 * @author Codex
 * @param [IN] strPath Source path.
 * @param [OUT] strContent File bytes.
 * @return True when the complete file is read.
 */
static bool ReadBinaryFile(const std::string &strPath, std::string &strContent)
{
    std::ifstream stInput(strPath.c_str(), std::ios::binary);
    if (!stInput.is_open())
    {
        return false;
    }

    stInput.seekg(0, std::ios::end);
    const std::streamoff nLength = stInput.tellg();
    if (nLength <= 0)
    {
        return false;
    }
    stInput.seekg(0, std::ios::beg);
    strContent.resize(static_cast<std::size_t>(nLength));
    stInput.read(&strContent[0], nLength);
    if (stInput.gcount() != nLength)
    {
        strContent.clear();
        return false;
    }
    return true;
}

/**
 * @brief Returns the parent directory component of a path.
 * @author Codex
 * @param [IN] strPath Input path.
 * @return Parent directory or an empty string.
 */
static std::string GetDirectoryName(const std::string &strPath)
{
    const std::size_t uPosition = strPath.find_last_of("/\\");
    return uPosition == std::string::npos ? std::string() : strPath.substr(0, uPosition);
}

/**
 * @brief Returns whether one path currently names a directory.
 * @author Codex
 * @param [IN] strPath Directory path.
 * @return True when the directory exists.
 */
static bool IsDirectory(const std::string &strPath)
{
    struct stat stInfo;
    if (stat(strPath.c_str(), &stInfo) != 0)
    {
        return false;
    }
#if defined(_WIN32)
    return (stInfo.st_mode & _S_IFDIR) != 0;
#else
    return S_ISDIR(stInfo.st_mode);
#endif
}

/**
 * @brief Creates one directory level and tolerates an existing directory.
 * @author Codex
 * @param [IN] strPath Directory path.
 * @return True when the directory exists after the operation.
 */
static bool CreateDirectoryLevel(const std::string &strPath)
{
    if (strPath.empty() || IsDirectory(strPath))
    {
        return true;
    }
#if defined(_WIN32)
    const int nResult = _mkdir(strPath.c_str());
#else
    const int nResult = mkdir(strPath.c_str(), 0755);
#endif
    return nResult == 0 || (errno == EEXIST && IsDirectory(strPath));
}

/**
 * @brief Recursively creates a directory path without shell execution.
 * @author Codex
 * @param [IN] strDirectory Directory path.
 * @return True when the complete path exists.
 */
static bool EnsureDirectory(const std::string &strDirectory)
{
    if (strDirectory.empty() || IsDirectory(strDirectory))
    {
        return true;
    }

    std::string strCurrent;
    std::size_t uPosition = 0;
#if defined(_WIN32)
    if (strDirectory.size() >= 2 && strDirectory[1] == ':')
    {
        strCurrent.assign(strDirectory, 0, 2);
        uPosition = 2;
    }
#endif
    if (uPosition < strDirectory.size() &&
        (strDirectory[uPosition] == '/' || strDirectory[uPosition] == '\\'))
    {
        strCurrent.push_back(strDirectory[uPosition]);
        ++uPosition;
    }

    while (uPosition <= strDirectory.size())
    {
        const std::size_t uSeparator = strDirectory.find_first_of("/\\", uPosition);
        const std::size_t uLength = uSeparator == std::string::npos
                                        ? strDirectory.size() - uPosition
                                        : uSeparator - uPosition;
        if (uLength > 0)
        {
            if (!strCurrent.empty() && strCurrent.back() != '/' && strCurrent.back() != '\\')
            {
                strCurrent.push_back('/');
            }
            strCurrent.append(strDirectory, uPosition, uLength);
            if (!CreateDirectoryLevel(strCurrent))
            {
                return false;
            }
        }
        if (uSeparator == std::string::npos)
        {
            break;
        }
        uPosition = uSeparator + 1;
    }
    return IsDirectory(strDirectory);
}

/**
 * @brief Returns a regular file size or a negative value on failure.
 * @author Codex
 * @param [IN] strPath File path.
 * @return File size in bytes or negative one.
 */
static std::int64_t GetFileSize(const std::string &strPath)
{
    struct stat stInfo;
    if (stat(strPath.c_str(), &stInfo) != 0)
    {
        return -1;
    }
    return static_cast<std::int64_t>(stInfo.st_size);
}

/**
 * @brief Extracts a host, port and path from an HTTP URL.
 * @author Codex
 * @return True when the URL uses HTTP and contains a host.
 */
static bool ParseHttpUrl(const std::string &strUrl,
                         std::string &strHost,
                         int &nPort,
                         std::string &strPath)
{
    constexpr const char *pHttpPrefix = "http://";
    constexpr const char *pHttpsPrefix = "https://";
    if (strUrl.compare(0, std::strlen(pHttpsPrefix), pHttpsPrefix) == 0)
    {
        return false;
    }
    if (strUrl.compare(0, std::strlen(pHttpPrefix), pHttpPrefix) != 0)
    {
        return false;
    }

    const std::size_t uAuthorityStart = std::strlen(pHttpPrefix);
    const std::size_t uPathPosition = strUrl.find('/', uAuthorityStart);
    const std::string strAuthority = uPathPosition == std::string::npos
                                         ? strUrl.substr(uAuthorityStart)
                                         : strUrl.substr(uAuthorityStart, uPathPosition - uAuthorityStart);
    strPath = uPathPosition == std::string::npos ? "/" : strUrl.substr(uPathPosition);
    if (strAuthority.empty())
    {
        return false;
    }

    const std::size_t uColonPosition = strAuthority.rfind(':');
    if (uColonPosition == std::string::npos)
    {
        strHost = strAuthority;
        nPort = 80;
        return true;
    }

    strHost = strAuthority.substr(0, uColonPosition);
    nPort = std::atoi(strAuthority.substr(uColonPosition + 1).c_str());
    return !strHost.empty() && nPort > 0 && nPort <= 65535;
}

/**
 * @brief Replaces characters unsafe for an uploaded file name.
 * @author Codex
 * @param [IN] strValue Raw file-name component.
 * @return Sanitized component.
 */
static std::string SanitizeFileName(const std::string &strValue)
{
    std::string strResult = strValue;
    for (char &chValue : strResult)
    {
        const unsigned char byValue = static_cast<unsigned char>(chValue);
        if (!std::isalnum(byValue) && chValue != '-' && chValue != '_' && chValue != '.')
        {
            chValue = '_';
        }
    }
    return strResult;
}

/**
 * @brief Extracts the final path component from a local image path.
 * @author Codex
 * @param [IN] strPath Local image path.
 * @return Base-name component or an empty string.
 */
static std::string GetPathBaseName(const std::string &strPath)
{
    const std::size_t uPosition = strPath.find_last_of("/\\");
    return uPosition == std::string::npos ? strPath : strPath.substr(uPosition + 1U);
}

/**
 * @brief Checks whether all characters in a string are decimal digits.
 * @author Codex
 * @param [IN] strValue Candidate text.
 * @return True when the text is nonempty and contains only digits.
 */
static bool IsDigitString(const std::string &strValue)
{
    if (strValue.empty())
    {
        return false;
    }
    for (char chValue : strValue)
    {
        if (std::isdigit(static_cast<unsigned char>(chValue)) == 0)
        {
            return false;
        }
    }
    return true;
}

/**
 * @brief Converts one local epoch time to a thread-safe local calendar value.
 * @author Codex
 * @param [IN] stTime Epoch time.
 * @param [OUT] stLocalTime Local calendar output.
 * @return True when conversion succeeds.
 */
static bool ConvertLocalTime(std::time_t stTime, struct tm &stLocalTime)
{
#if defined(_WIN32)
    return localtime_s(&stLocalTime, &stTime) == 0;
#else
    return localtime_r(&stTime, &stLocalTime) != nullptr;
#endif
}

/**
 * @brief Parses yyyyMMdd_HHmmssSSS from an IPC capture file name.
 * @author Codex
 * @param [IN] strImagePath Capture image path.
 * @param [OUT] llTimestampMs Parsed Unix timestamp in milliseconds.
 * @return True when the file-name timestamp is valid in the device timezone.
 */
static bool ParseCaptureTimestampMs(const std::string &strImagePath,
                                    std::int64_t &llTimestampMs)
{
    llTimestampMs = 0;
    const std::string strName = GetPathBaseName(strImagePath);
    if (strName.size() < 19U || strName[8] != '_' || strName[18] != '_')
    {
        return false;
    }

    const std::string strDate = strName.substr(0, 8);
    const std::string strTime = strName.substr(9, 9);
    if (!IsDigitString(strDate) || !IsDigitString(strTime))
    {
        return false;
    }

    const int nYear = std::atoi(strDate.substr(0, 4).c_str());
    const int nMonth = std::atoi(strDate.substr(4, 2).c_str());
    const int nDay = std::atoi(strDate.substr(6, 2).c_str());
    const int nHour = std::atoi(strTime.substr(0, 2).c_str());
    const int nMinute = std::atoi(strTime.substr(2, 2).c_str());
    const int nSecond = std::atoi(strTime.substr(4, 2).c_str());
    const int nMillisecond = std::atoi(strTime.substr(6, 3).c_str());
    if (nYear < 1970 || nMonth < 1 || nMonth > 12 || nDay < 1 || nDay > 31 ||
        nHour < 0 || nHour > 23 || nMinute < 0 || nMinute > 59 ||
        nSecond < 0 || nSecond > 59 || nMillisecond < 0 || nMillisecond > 999)
    {
        return false;
    }

    struct tm stCalendarTime;
    std::memset(&stCalendarTime, 0, sizeof(stCalendarTime));
    stCalendarTime.tm_year = nYear - 1900;
    stCalendarTime.tm_mon = nMonth - 1;
    stCalendarTime.tm_mday = nDay;
    stCalendarTime.tm_hour = nHour;
    stCalendarTime.tm_min = nMinute;
    stCalendarTime.tm_sec = nSecond;
    stCalendarTime.tm_isdst = -1;

    const std::time_t stEpochTime = std::mktime(&stCalendarTime);
    if (stEpochTime < 0)
    {
        return false;
    }

    struct tm stVerifiedTime;
    std::memset(&stVerifiedTime, 0, sizeof(stVerifiedTime));
    if (!ConvertLocalTime(stEpochTime, stVerifiedTime) ||
        stVerifiedTime.tm_year != stCalendarTime.tm_year ||
        stVerifiedTime.tm_mon != stCalendarTime.tm_mon ||
        stVerifiedTime.tm_mday != stCalendarTime.tm_mday ||
        stVerifiedTime.tm_hour != stCalendarTime.tm_hour ||
        stVerifiedTime.tm_min != stCalendarTime.tm_min ||
        stVerifiedTime.tm_sec != stCalendarTime.tm_sec)
    {
        return false;
    }

    llTimestampMs = static_cast<std::int64_t>(stEpochTime) * 1000LL +
                    static_cast<std::int64_t>(nMillisecond);
    return true;
}

/**
 * @brief Returns the current Unix timestamp in milliseconds.
 * @author Codex
 * @return Current system time in milliseconds.
 */
static std::int64_t GetCurrentTimestampMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}
}

bool CPlatformHttpClient::Configure(const Config_S &stConfig)
{
    if (stConfig.strHost.empty() || stConfig.nPort <= 0 || stConfig.nPort > 65535 ||
        stConfig.strUser.empty() || stConfig.strPassword.empty())
    {
        return false;
    }

    std::lock_guard<std::mutex> stSessionLock(m_mtxSession);
    m_stConfig = stConfig;
    m_strAccessToken.clear();
    return true;
}

bool CPlatformHttpClient::Login(std::string &strError)
{
    strError.clear();
    Config_S stConfig;
    {
        std::lock_guard<std::mutex> stSessionLock(m_mtxSession);
        stConfig = m_stConfig;
    }

    const std::string strEncodedPassword = Base64Encode(stConfig.strPassword);
    if (strEncodedPassword.empty())
    {
        strError = "platform password encoding failed";
        return false;
    }

    const std::string strBody = "user=" + EncodeFormValue(stConfig.strUser) +
                                "&password=" + EncodeFormValue(strEncodedPassword);
    httplib::Client stClient(stConfig.strHost, stConfig.nPort);
    stClient.set_connection_timeout(5, 0);
    stClient.set_read_timeout(15, 0);
    const httplib::Headers stHeaders = {
        {"Content-Type", "application/x-www-form-urlencoded"}};
    const auto pResponse = stClient.Post(PLATFORM_HTTP_LOGIN_PATH,
                                         stHeaders,
                                         strBody,
                                         "application/x-www-form-urlencoded");
    if (!pResponse)
    {
        strError = "platform login returned no response";
        return false;
    }
    if (pResponse->status < 200 || pResponse->status >= 300)
    {
        strError = "platform login HTTP status " + std::to_string(pResponse->status);
        return false;
    }

    cJSON *pRoot = cJSON_Parse(pResponse->body.c_str());
    if (pRoot == nullptr)
    {
        strError = "platform login response is not JSON";
        return false;
    }

    const std::string strStatus = GetJsonString(pRoot, "status");
    cJSON *pData = cJSON_GetObjectItemCaseSensitive(pRoot, "data");
    const std::string strToken = cJSON_IsObject(pData)
                                     ? GetJsonString(pData, "access_token")
                                     : std::string();
    cJSON_Delete(pRoot);
    if (strStatus != "success" || strToken.empty())
    {
        strError = "platform login business response failed";
        return false;
    }

    {
        std::lock_guard<std::mutex> stSessionLock(m_mtxSession);
        m_strAccessToken = strToken;
    }
    NETSDK_LOG_MESSAGE_INFO("Platform HTTP authentication succeeded: host=%s:%d",
                            stConfig.strHost.c_str(),
                            stConfig.nPort);
    return true;
}

bool CPlatformHttpClient::RegisterDevice(const NET_PlatformDeviceProfile_S &stDevice,
                                         const std::string &strLiveUrl,
                                         const std::string &strProtocol,
                                         std::string &strError)
{
    strError.clear();
    const std::string strToken = GetAccessToken();
    if (strToken.empty())
    {
        strError = "platform access token is empty";
        return false;
    }

    Config_S stConfig;
    {
        std::lock_guard<std::mutex> stSessionLock(m_mtxSession);
        stConfig = m_stConfig;
    }

    cJSON *pRoot = cJSON_CreateObject();
    if (pRoot == nullptr)
    {
        strError = "device registration JSON allocation failed";
        return false;
    }

    cJSON_AddStringToObject(pRoot,
                            "sn",
                            CopyAbiString(stDevice.strSerialNumber,
                                          sizeof(stDevice.strSerialNumber)).c_str());
    cJSON_AddStringToObject(pRoot,
                            "name",
                            CopyAbiString(stDevice.strDeviceName,
                                          sizeof(stDevice.strDeviceName)).c_str());
    cJSON_AddStringToObject(pRoot,
                            "version",
                            CopyAbiString(stDevice.strFirmwareVersion,
                                          sizeof(stDevice.strFirmwareVersion)).c_str());
    cJSON_AddStringToObject(pRoot,
                            "mac_address",
                            CopyAbiString(stDevice.strMacAddress,
                                          sizeof(stDevice.strMacAddress)).c_str());
    cJSON_AddStringToObject(pRoot,
                            "ip",
                            CopyAbiString(stDevice.strLocalIp,
                                          sizeof(stDevice.strLocalIp)).c_str());
    cJSON_AddNumberToObject(pRoot, "port", stDevice.nServicePort);
    cJSON_AddStringToObject(pRoot, "account", stConfig.strUser.c_str());
    cJSON_AddStringToObject(pRoot, "password", stConfig.strPassword.c_str());
    cJSON_AddStringToObject(pRoot, "live_url", strLiveUrl.c_str());
    cJSON_AddStringToObject(pRoot, "playback_url", "");
    cJSON_AddStringToObject(pRoot, "protocol", strProtocol.c_str());
    cJSON_AddStringToObject(pRoot,
                            "resolution",
                            CopyAbiString(stDevice.strResolution,
                                          sizeof(stDevice.strResolution)).c_str());
    cJSON_AddStringToObject(pRoot,
                            "storage",
                            CopyAbiString(stDevice.strStorage,
                                          sizeof(stDevice.strStorage)).c_str());
    cJSON_AddStringToObject(pRoot,
                            "use_storage",
                            CopyAbiString(stDevice.strUseStorage,
                                          sizeof(stDevice.strUseStorage)).c_str());
    cJSON_AddStringToObject(pRoot,
                            "location",
                            CopyAbiString(stDevice.strLocation,
                                          sizeof(stDevice.strLocation)).c_str());
    cJSON_AddNumberToObject(pRoot, "type_id", 1);
    const std::string strBody = PrintJson(pRoot);
    cJSON_Delete(pRoot);
    if (strBody.empty())
    {
        strError = "device registration JSON serialization failed";
        return false;
    }

    httplib::Client stClient(stConfig.strHost, stConfig.nPort);
    stClient.set_connection_timeout(5, 0);
    stClient.set_read_timeout(15, 0);
    const httplib::Headers stHeaders = {
        {"Content-Type", "application/json"},
        {"Authorization", "Bearer " + strToken}};
    const auto pResponse = stClient.Post(PLATFORM_HTTP_REGISTER_PATH,
                                         stHeaders,
                                         strBody,
                                         "application/json");
    if (!pResponse)
    {
        strError = "device registration returned no response";
        return false;
    }
    if (pResponse->status < 200 || pResponse->status >= 300)
    {
        strError = "device registration HTTP status " + std::to_string(pResponse->status);
        return false;
    }

    cJSON *pResponseRoot = cJSON_Parse(pResponse->body.c_str());
    if (pResponseRoot == nullptr)
    {
        strError = "device registration response is not JSON";
        return false;
    }

    cJSON *pStatusCode = cJSON_GetObjectItemCaseSensitive(pResponseRoot, "status_code");
    const std::string strStatus = GetJsonString(pResponseRoot, "status");
    const bool bBusinessSuccess =
        (!cJSON_IsNumber(pStatusCode) || pStatusCode->valueint == 200) &&
        (strStatus.empty() || strStatus == "success");
    cJSON_Delete(pResponseRoot);
    if (!bBusinessSuccess)
    {
        strError = "device registration business response failed";
        return false;
    }

    NETSDK_LOG_MESSAGE_INFO("Platform HTTP device registration succeeded: sn=%s protocol=%s",
                            stDevice.strSerialNumber,
                            strProtocol.c_str());
    return true;
}

bool CPlatformHttpClient::UploadEventImage(const EventImageRequest_S &stRequest,
                                           EventImageResponse_S &stResponse,
                                           std::string &strError)
{
    stResponse = EventImageResponse_S();
    strError.clear();
    if (stRequest.strDeviceSn.empty() || stRequest.strImagePath.empty())
    {
        strError = "event image request is incomplete";
        return false;
    }

    const std::string strToken = GetAccessToken();
    if (strToken.empty())
    {
        strError = "platform access token is empty";
        return false;
    }

    std::string strImageContent;
    if (!ReadBinaryFile(stRequest.strImagePath, strImageContent))
    {
        strError = "event image file could not be read";
        return false;
    }

    Config_S stConfig;
    {
        std::lock_guard<std::mutex> stSessionLock(m_mtxSession);
        stConfig = m_stConfig;
    }

    std::int64_t llUploadTimestampMs = 0;
    if (!ParseCaptureTimestampMs(stRequest.strImagePath, llUploadTimestampMs))
    {
        llUploadTimestampMs = stRequest.llTimestampMs > 0
                                  ? stRequest.llTimestampMs
                                  : GetCurrentTimestampMs();
    }
    const std::string strTimestamp = std::to_string(llUploadTimestampMs);
    std::string strFileName = stRequest.strFileName;
    if (strFileName.empty())
    {
        strFileName = stRequest.strDeviceSn + "_" + std::to_string(stRequest.nEventType) +
                      "_" + strTimestamp + "_1.jpg";
    }
    strFileName = SanitizeFileName(strFileName);

    httplib::Client stClient(stConfig.strHost, stConfig.nPort);
    stClient.set_connection_timeout(5, 0);
    stClient.set_read_timeout(30, 0);
    const httplib::Headers stHeaders = {
        {"Authorization", "Bearer " + strToken}};
    const httplib::UploadFormDataItems aItems = {
        {"device_sn", stRequest.strDeviceSn, "", ""},
        {"event_type", std::to_string(stRequest.nEventType), "", ""},
        {"time", strTimestamp, "", ""},
        {"up_screen", strImageContent, strFileName, "image/jpeg"}};
    const auto pResponse = stClient.Post(PLATFORM_HTTP_EVENT_IMAGE_PATH, stHeaders, aItems);
    if (!pResponse)
    {
        strError = "event image upload returned no response";
        return false;
    }

    stResponse.nStatusCode = pResponse->status;
    stResponse.strImagePath = stRequest.strImagePath;
    stResponse.strFileName = strFileName;
    cJSON *pRoot = cJSON_Parse(pResponse->body.c_str());
    if (pRoot != nullptr)
    {
        cJSON *pStatusCode = cJSON_GetObjectItemCaseSensitive(pRoot, "status_code");
        if (cJSON_IsNumber(pStatusCode))
        {
            stResponse.nStatusCode = pStatusCode->valueint;
        }
        stResponse.strStatus = GetJsonString(pRoot, "status");
        stResponse.strMessage = GetJsonString(pRoot, "message");
        stResponse.strImageUrl = GetJsonString(pRoot, "image_url");
        cJSON *pData = cJSON_GetObjectItemCaseSensitive(pRoot, "data");
        if (stResponse.strImageUrl.empty() && cJSON_IsObject(pData))
        {
            stResponse.strImageUrl = GetJsonString(pData, "image_url");
            if (stResponse.strImageUrl.empty())
            {
                stResponse.strImageUrl = GetJsonString(pData, "url");
            }
        }
        cJSON_Delete(pRoot);
    }

    const bool bHttpSuccess = pResponse->status >= 200 && pResponse->status < 300;
    const bool bBusinessCodeSuccess =
        stResponse.nStatusCode == 0 || stResponse.nStatusCode == 200 ||
        stResponse.nStatusCode == pResponse->status;
    const bool bBusinessSuccess = bBusinessCodeSuccess &&
                                  (stResponse.strStatus.empty() ||
                                   stResponse.strStatus == "success");
    if (!bHttpSuccess || !bBusinessSuccess)
    {
        strError = "event image upload failed";
        return false;
    }

    NETSDK_LOG_MESSAGE_INFO("Platform event image uploaded: event=%d file=%s size=%zu",
                            stRequest.nEventType,
                            strFileName.c_str(),
                            strImageContent.size());
    return true;
}

bool CPlatformHttpClient::DownloadFile(const std::string &strUrl,
                                       const std::string &strLocalPath,
                                       std::int64_t llExpectedSize,
                                       std::string &strError) const
{
    strError.clear();
    if (strUrl.empty() || strLocalPath.empty() || llExpectedSize < 0)
    {
        strError = "download parameters are invalid";
        return false;
    }

    Config_S stConfig;
    std::string strToken;
    {
        std::lock_guard<std::mutex> stSessionLock(m_mtxSession);
        stConfig = m_stConfig;
        strToken = m_strAccessToken;
    }

    std::string strResolvedUrl = strUrl;
    if (strResolvedUrl.compare(0, 7, "http://") != 0 &&
        strResolvedUrl.compare(0, 8, "https://") != 0)
    {
        strResolvedUrl = "http://" + stConfig.strHost + ":" +
                         std::to_string(stConfig.nPort) +
                         (strUrl.empty() || strUrl[0] == '/' ? "" : "/") + strUrl;
    }

    std::string strHost;
    std::string strPath;
    int nPort = 0;
    if (!ParseHttpUrl(strResolvedUrl, strHost, nPort, strPath))
    {
        strError = "only HTTP image download is supported by this SDK build";
        return false;
    }

    const std::string strDirectory = GetDirectoryName(strLocalPath);
    if (!EnsureDirectory(strDirectory))
    {
        strError = "download directory could not be created";
        return false;
    }

    httplib::Client stClient(strHost, nPort);
    stClient.set_connection_timeout(5, 0);
    stClient.set_read_timeout(30, 0);
    httplib::Headers stHeaders;
    if (!strToken.empty() && strHost == stConfig.strHost)
    {
        stHeaders.emplace("Authorization", "Bearer " + strToken);
    }
    const auto pResponse = stHeaders.empty()
                               ? stClient.Get(strPath)
                               : stClient.Get(strPath, stHeaders);
    if (!pResponse)
    {
        strError = "image download returned no response";
        return false;
    }
    if (pResponse->status != 200 || pResponse->body.empty())
    {
        strError = "image download HTTP status " + std::to_string(pResponse->status);
        return false;
    }
    if (llExpectedSize > 0 &&
        static_cast<std::int64_t>(pResponse->body.size()) != llExpectedSize)
    {
        strError = "downloaded image size mismatch";
        return false;
    }

    const std::string strTemporaryPath = strLocalPath + ".tmp";
    {
        std::ofstream stOutput(strTemporaryPath.c_str(), std::ios::binary | std::ios::trunc);
        if (!stOutput.is_open())
        {
            strError = "download temporary file could not be opened";
            return false;
        }
        stOutput.write(pResponse->body.data(),
                       static_cast<std::streamsize>(pResponse->body.size()));
        if (!stOutput.good())
        {
            stOutput.close();
            std::remove(strTemporaryPath.c_str());
            strError = "download temporary file write failed";
            return false;
        }
    }

    if (llExpectedSize > 0 && GetFileSize(strTemporaryPath) != llExpectedSize)
    {
        std::remove(strTemporaryPath.c_str());
        strError = "downloaded file validation failed";
        return false;
    }

#if defined(_WIN32)
    std::remove(strLocalPath.c_str());
#endif
    if (std::rename(strTemporaryPath.c_str(), strLocalPath.c_str()) != 0)
    {
        std::remove(strTemporaryPath.c_str());
        strError = "downloaded file replacement failed";
        return false;
    }

    NETSDK_LOG_MESSAGE_INFO("Platform image downloaded: path=%s size=%zu",
                            strLocalPath.c_str(),
                            pResponse->body.size());
    return true;
}

void CPlatformHttpClient::ClearSession()
{
    std::lock_guard<std::mutex> stSessionLock(m_mtxSession);
    m_strAccessToken.clear();
}

bool CPlatformHttpClient::IsAuthenticated() const
{
    std::lock_guard<std::mutex> stSessionLock(m_mtxSession);
    return !m_strAccessToken.empty();
}

std::string CPlatformHttpClient::GetAccessToken() const
{
    std::lock_guard<std::mutex> stSessionLock(m_mtxSession);
    return m_strAccessToken;
}
