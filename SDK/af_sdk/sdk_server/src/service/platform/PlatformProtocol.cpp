/**
 * @file PlatformProtocol.cpp
 * @author Codex
 * @date 2026-08-22
 * @brief Implements MQTT topic and JSON protocol compatibility for the SDK runtime.
 * @change 2026-08-22 Codex Initial implementation for platform migration.
 */

#include "PlatformProtocol.h"

#include <chrono>
#include <cstring>
#include <sstream>

#include "PlatformRegisterCrypto.h"
#include "cJSON.h"

namespace
{
constexpr const char *PLATFORM_PROTOCOL_DEVICE_PREFIX = "device/";
constexpr const char *PLATFORM_PROTOCOL_REGISTER_COMMAND = "NET_DEVICE_REGISTER";
constexpr const char *PLATFORM_PROTOCOL_STATUS_COMMAND = "NET_TV_DEVICE_STATUS";

/**
 * @brief Copies a bounded ABI character array into a C++ string.
 * @author Codex
 * @param [IN] pValue Source character array.
 * @param [IN] uCapacity Source capacity in bytes.
 * @return Bounded string value.
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
 * @brief Serializes one cJSON node without changing its ownership.
 * @author Codex
 * @param [IN] pRoot JSON node.
 * @return Compact JSON text or an empty string.
 */
static std::string PrintJson(cJSON *pRoot)
{
    char *pText = pRoot != nullptr ? cJSON_PrintUnformatted(pRoot) : nullptr;
    std::string strResult = pText != nullptr ? pText : std::string();
    if (pText != nullptr)
    {
        cJSON_free(pText);
    }
    return strResult;
}

/**
 * @brief Parses a Data JSON value and attaches a safe object when parsing fails.
 * @author Codex
 * @param [IN] pRoot Envelope root.
 * @param [IN] strDataJson Data JSON text.
 * @return True when a Data node is attached.
 */
static bool AddDataJson(cJSON *pRoot, const std::string &strDataJson)
{
    if (pRoot == nullptr)
    {
        return false;
    }

    cJSON *pData = cJSON_Parse(strDataJson.empty() ? "{}" : strDataJson.c_str());
    if (pData == nullptr)
    {
        pData = cJSON_CreateObject();
    }
    if (pData == nullptr)
    {
        return false;
    }
    cJSON_AddItemToObject(pRoot, "Data", pData);
    return true;
}

/**
 * @brief Builds the exact credential plaintext agreed with the platform.
 * @author Codex
 * @param [IN] stInput Registration input.
 * @return Compact credential JSON.
 */
static std::string BuildCredentialPlaintext(
    const CPlatformProtocol::RegisterInput_S &stInput)
{
    cJSON *pRoot = cJSON_CreateObject();
    if (pRoot == nullptr)
    {
        return std::string();
    }
    cJSON_AddStringToObject(pRoot, "RtspUrl", stInput.strRtspUrl.c_str());
    cJSON_AddStringToObject(pRoot, "Username", stInput.strRtspAccount.c_str());
    cJSON_AddStringToObject(pRoot, "Password", stInput.strRtspPassword.c_str());
    const std::string strResult = PrintJson(pRoot);
    cJSON_Delete(pRoot);
    return strResult;
}

/**
 * @brief Builds the immutable AAD contract used by platform decryption.
 * @author Codex
 * @param [IN] stInput Registration input.
 * @param [IN] strRequestId Generated request identifier.
 * @return AAD text in protocol field order.
 */
static std::string BuildCredentialAad(
    const CPlatformProtocol::RegisterInput_S &stInput,
    const std::string &strRequestId)
{
    std::ostringstream stOutput;
    stOutput << PLATFORM_PROTOCOL_REGISTER_COMMAND << "|1|"
             << stInput.strDeviceSn << "|"
             << strRequestId << "|"
             << stInput.llTimestampMs << "|"
             << stInput.strUplinkType << "|"
             << stInput.strStreamMode;
    return stOutput.str();
}

/**
 * @brief Adds a nonempty string field to a JSON object.
 * @author Codex
 * @param [IN] pRoot JSON object.
 * @param [IN] pName Field name.
 * @param [IN] strValue Field value.
 * @return No return value.
 */
static void AddStringIfNotEmpty(cJSON *pRoot,
                                const char *pName,
                                const std::string &strValue)
{
    if (pRoot != nullptr && !strValue.empty())
    {
        cJSON_AddStringToObject(pRoot, pName, strValue.c_str());
    }
}

/**
 * @brief Adds or replaces one string field in a JSON object.
 * @author Codex
 * @param [IN] pRoot JSON object.
 * @param [IN] pName Field name.
 * @param [IN] strValue Field value.
 * @return True when the field is stored.
 */
static bool SetJsonString(cJSON *pRoot,
                          const char *pName,
                          const std::string &strValue)
{
    cJSON *pValue = cJSON_CreateString(strValue.c_str());
    if (pRoot == nullptr || pValue == nullptr)
    {
        if (pValue != nullptr)
        {
            cJSON_Delete(pValue);
        }
        return false;
    }
    if (cJSON_GetObjectItemCaseSensitive(pRoot, pName) != nullptr)
    {
        if (cJSON_ReplaceItemInObjectCaseSensitive(pRoot, pName, pValue) == 0)
        {
            cJSON_Delete(pValue);
            return false;
        }
        return true;
    }
    cJSON_AddItemToObject(pRoot, pName, pValue);
    return true;
}

/**
 * @brief Adds or replaces one numeric field in a JSON object.
 * @author Codex
 * @param [IN] pRoot JSON object.
 * @param [IN] pName Field name.
 * @param [IN] dValue Numeric field value.
 * @return True when the field is stored.
 */
static bool SetJsonNumber(cJSON *pRoot, const char *pName, double dValue)
{
    cJSON *pValue = cJSON_CreateNumber(dValue);
    if (pRoot == nullptr || pValue == nullptr)
    {
        if (pValue != nullptr)
        {
            cJSON_Delete(pValue);
        }
        return false;
    }
    if (cJSON_GetObjectItemCaseSensitive(pRoot, pName) != nullptr)
    {
        if (cJSON_ReplaceItemInObjectCaseSensitive(pRoot, pName, pValue) == 0)
        {
            cJSON_Delete(pValue);
            return false;
        }
        return true;
    }
    cJSON_AddItemToObject(pRoot, pName, pValue);
    return true;
}
}

std::string CPlatformProtocol::BuildCommandTopic(const std::string &strDeviceSn)
{
    return std::string(PLATFORM_PROTOCOL_DEVICE_PREFIX) + strDeviceSn + "/command";
}

std::string CPlatformProtocol::BuildResponseTopic(const std::string &strDeviceSn)
{
    return std::string(PLATFORM_PROTOCOL_DEVICE_PREFIX) + strDeviceSn + "/response";
}

std::string CPlatformProtocol::BuildEventTopic(const std::string &strDeviceSn)
{
    return std::string(PLATFORM_PROTOCOL_DEVICE_PREFIX) + strDeviceSn + "/event";
}

std::string CPlatformProtocol::BuildStatusTopic(const std::string &strDeviceSn)
{
    return std::string(PLATFORM_PROTOCOL_DEVICE_PREFIX) + strDeviceSn + "/status";
}

std::string CPlatformProtocol::BuildRegisterTopic(const std::string &strDeviceSn)
{
    return std::string(PLATFORM_PROTOCOL_DEVICE_PREFIX) + strDeviceSn + "/register";
}

bool CPlatformProtocol::ParseCommand(const std::string &strPayload,
                                     Command_S &stCommand,
                                     std::string &strError)
{
    stCommand = Command_S();
    strError.clear();
    cJSON *pRoot = cJSON_Parse(strPayload.c_str());
    if (!cJSON_IsObject(pRoot))
    {
        if (pRoot != nullptr)
        {
            cJSON_Delete(pRoot);
        }
        strError = "MQTT command payload is not a JSON object";
        return false;
    }

    cJSON *pCommand = cJSON_GetObjectItemCaseSensitive(pRoot, "Command");
    cJSON *pRequestId = cJSON_GetObjectItemCaseSensitive(pRoot, "RequestId");
    if (!cJSON_IsString(pCommand) || pCommand->valuestring == nullptr ||
        pCommand->valuestring[0] == '\0' || !cJSON_IsString(pRequestId) ||
        pRequestId->valuestring == nullptr || pRequestId->valuestring[0] == '\0')
    {
        cJSON_Delete(pRoot);
        strError = "MQTT command is missing Command or RequestId";
        return false;
    }

    stCommand.strCommand.assign(pCommand->valuestring);
    stCommand.strRequestId.assign(pRequestId->valuestring);
    cJSON *pData = cJSON_GetObjectItemCaseSensitive(pRoot, "Data");
    if (pData != nullptr)
    {
        stCommand.strData = PrintJson(pData);
        if (stCommand.strData.empty())
        {
            cJSON_Delete(pRoot);
            strError = "MQTT command Data serialization failed";
            return false;
        }
    }
    cJSON_Delete(pRoot);
    return true;
}

std::string CPlatformProtocol::BuildResponse(const std::string &strCommand,
                                             const std::string &strRequestId,
                                             int nReturn,
                                             const std::string &strDataJson,
                                             const std::string &strMessage)
{
    cJSON *pRoot = cJSON_CreateObject();
    if (pRoot == nullptr)
    {
        return std::string();
    }
    cJSON_AddStringToObject(pRoot, "Command", strCommand.c_str());
    cJSON_AddStringToObject(pRoot, "RequestId", strRequestId.c_str());
    cJSON_AddNumberToObject(pRoot, "Return", nReturn);
    if (!AddDataJson(pRoot, strDataJson))
    {
        cJSON_Delete(pRoot);
        return std::string();
    }
    AddStringIfNotEmpty(pRoot, "Message", strMessage);
    const std::string strResult = PrintJson(pRoot);
    cJSON_Delete(pRoot);
    return strResult;
}

std::string CPlatformProtocol::BuildEvent(const std::string &strCommand,
                                          const std::string &strRequestId,
                                          const std::string &strDataJson)
{
    cJSON *pRoot = cJSON_CreateObject();
    if (pRoot == nullptr)
    {
        return std::string();
    }
    cJSON_AddStringToObject(pRoot, "Command", strCommand.c_str());
    cJSON_AddStringToObject(pRoot, "RequestId", strRequestId.c_str());
    if (!AddDataJson(pRoot, strDataJson))
    {
        cJSON_Delete(pRoot);
        return std::string();
    }
    const std::string strResult = PrintJson(pRoot);
    cJSON_Delete(pRoot);
    return strResult;
}

std::string CPlatformProtocol::BuildStatus(const std::string &strDeviceSn,
                                           bool bOnline,
                                           const std::string &strReason,
                                           std::int64_t llTimestampMs)
{
    cJSON *pRoot = cJSON_CreateObject();
    if (pRoot == nullptr)
    {
        return std::string();
    }
    cJSON_AddStringToObject(pRoot, "Command", PLATFORM_PROTOCOL_STATUS_COMMAND);
    const std::string strRequestId = llTimestampMs > 0
                                         ? "status-" + strDeviceSn + "-" +
                                               std::to_string(llTimestampMs)
                                         : "lwt";
    cJSON_AddStringToObject(pRoot, "RequestId", strRequestId.c_str());
    cJSON *pData = cJSON_AddObjectToObject(pRoot, "Data");
    if (pData != nullptr)
    {
        cJSON_AddStringToObject(pData, "Status", bOnline ? "online" : "offline");
        if (llTimestampMs > 0)
        {
            cJSON_AddStringToObject(pData,
                                    "Timestamp",
                                    std::to_string(llTimestampMs).c_str());
        }
        cJSON_AddStringToObject(pData, "Reason", strReason.c_str());
    }
    const std::string strResult = PrintJson(pRoot);
    cJSON_Delete(pRoot);
    return strResult;
}

bool CPlatformProtocol::BuildRegistration(const RegisterInput_S &stInput,
                                          std::string &strRequestId,
                                          std::string &strPayload,
                                          std::string &strWarning)
{
    strRequestId.clear();
    strPayload.clear();
    strWarning.clear();
    if (stInput.strDeviceSn.empty() || stInput.llTimestampMs <= 0 ||
        stInput.strUplinkType.empty() || stInput.strStreamMode.empty())
    {
        strWarning = "device registration input is incomplete";
        return false;
    }

    strRequestId = "register-" + stInput.strDeviceSn + "-" +
                   std::to_string(stInput.llTimestampMs);
    const bool bUseRtsp = stInput.strStreamMode == "rtsp";
    CPlatformRegisterCrypto::EncryptedCredential_S stEncryptedCredential;
    bool bCredentialEncrypted = false;
    if (bUseRtsp && !stInput.strRtspUrl.empty())
    {
        std::string strPlaintext = BuildCredentialPlaintext(stInput);
        const std::string strAad = BuildCredentialAad(stInput, strRequestId);
        bCredentialEncrypted = !strPlaintext.empty() &&
                               CPlatformRegisterCrypto::EncryptCredential(
                                   stInput.strPublicKeyPath,
                                   stInput.strPublicKeyId,
                                   strPlaintext,
                                   strAad,
                                   stEncryptedCredential,
                                   strWarning);
        CPlatformRegisterCrypto::CleanseString(strPlaintext);
    }
    else if (bUseRtsp)
    {
        strWarning = "RTSP URL unavailable";
    }

    cJSON *pRoot = cJSON_CreateObject();
    if (pRoot == nullptr)
    {
        return false;
    }
    cJSON_AddStringToObject(pRoot, "Command", PLATFORM_PROTOCOL_REGISTER_COMMAND);
    cJSON_AddStringToObject(pRoot, "RequestId", strRequestId.c_str());
    cJSON *pData = cJSON_AddObjectToObject(pRoot, "Data");
    if (pData == nullptr)
    {
        cJSON_Delete(pRoot);
        return false;
    }
    cJSON_AddStringToObject(pData, "Sn", stInput.strDeviceSn.c_str());
    cJSON_AddStringToObject(pData, "UplinkType", stInput.strUplinkType.c_str());
    cJSON_AddStringToObject(pData,
                            "UplinkInterface",
                            stInput.strUplinkInterface.c_str());
    cJSON_AddStringToObject(pData, "LocalIp", stInput.strLocalIp.c_str());
    cJSON_AddStringToObject(pData, "StreamMode", stInput.strStreamMode.c_str());
    cJSON_AddStringToObject(pData,
                            "Timestamp",
                            std::to_string(stInput.llTimestampMs).c_str());

    if (bCredentialEncrypted)
    {
        cJSON *pCredential = cJSON_AddObjectToObject(pData, "Credential");
        if (pCredential != nullptr)
        {
            cJSON_AddNumberToObject(pCredential, "Version", 1);
            cJSON_AddStringToObject(pCredential,
                                    "Algorithm",
                                    stEncryptedCredential.strAlgorithm.c_str());
            cJSON_AddStringToObject(pCredential,
                                    "KeyId",
                                    stEncryptedCredential.strKeyId.c_str());
            cJSON_AddStringToObject(pCredential,
                                    "EncryptedKey",
                                    stEncryptedCredential.strEncryptedKey.c_str());
            cJSON_AddStringToObject(pCredential,
                                    "Nonce",
                                    stEncryptedCredential.strNonce.c_str());
            cJSON_AddStringToObject(pCredential,
                                    "Ciphertext",
                                    stEncryptedCredential.strCiphertext.c_str());
            cJSON_AddStringToObject(pCredential,
                                    "Tag",
                                    stEncryptedCredential.strTag.c_str());
        }
    }
    else
    {
        cJSON_AddStringToObject(pData,
                                "CredentialState",
                                bUseRtsp ? "unavailable" : "not_required");
    }

    if (!bUseRtsp)
    {
        cJSON *pRtmp = cJSON_AddObjectToObject(pData, "Rtmp");
        if (pRtmp != nullptr)
        {
            cJSON_AddStringToObject(pRtmp, "Url", stInput.strRtmpUrl.c_str());
        }
    }

    strPayload = PrintJson(pRoot);
    cJSON_Delete(pRoot);
    return !strPayload.empty();
}

std::string CPlatformProtocol::BuildImageUploadData(
    const NET_PlatformEventReport_S &stEvent,
    const std::string &strAlarmRequestId,
    const CPlatformHttpClient::EventImageResponse_S &stResponse,
    bool bUploadOk,
    const std::string &strError)
{
    cJSON *pRoot = stEvent.pDataJson != nullptr && stEvent.pDataJson[0] != '\0'
                       ? cJSON_Parse(stEvent.pDataJson)
                       : nullptr;
    if (!cJSON_IsObject(pRoot))
    {
        if (pRoot != nullptr)
        {
            cJSON_Delete(pRoot);
        }
        pRoot = cJSON_CreateObject();
        if (pRoot == nullptr)
        {
            return std::string();
        }
    }

    SetJsonNumber(pRoot, "EventType", stEvent.nEventType);
    if (stEvent.pEventName != nullptr && stEvent.pEventName[0] != '\0')
    {
        SetJsonString(pRoot, "EventName", stEvent.pEventName);
    }
    SetJsonNumber(pRoot, "Channel", stEvent.nChannel);
    SetJsonString(pRoot, "Timestamp", std::to_string(stEvent.llTimestampMs));
    SetJsonString(pRoot, "AlarmRequestId", strAlarmRequestId);
    SetJsonNumber(pRoot, "UploadStatus", bUploadOk ? 1 : 0);
    if (!stResponse.strImagePath.empty())
    {
        SetJsonString(pRoot, "ImagePath", stResponse.strImagePath);
    }
    if (!stResponse.strFileName.empty())
    {
        SetJsonString(pRoot, "FileName", stResponse.strFileName);
    }
    if (!stResponse.strImageUrl.empty())
    {
        SetJsonString(pRoot, "ImageUrl", stResponse.strImageUrl);
    }
    if (stResponse.nStatusCode != 0)
    {
        SetJsonNumber(pRoot, "StatusCode", stResponse.nStatusCode);
    }
    if (!stResponse.strMessage.empty())
    {
        SetJsonString(pRoot, "Message", stResponse.strMessage);
    }
    if (!strError.empty())
    {
        SetJsonString(pRoot, "Error", strError);
    }

    const std::string strResult = PrintJson(pRoot);
    cJSON_Delete(pRoot);
    return strResult;
}

std::int64_t CPlatformProtocol::GetCurrentTimestampMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}
