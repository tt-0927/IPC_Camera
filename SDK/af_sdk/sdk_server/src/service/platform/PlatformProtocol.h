/**
 * @file PlatformProtocol.h
 * @author Codex
 * @date 2026-08-22
 * @brief Declares the MQTT topic and JSON protocol used by the SDK platform runtime.
 * @change 2026-08-22 Codex Initial implementation for platform migration.
 */
#pragma once

#include <cstdint>
#include <string>

#include "NetTVSDKServerInterface.h"
#include "PlatformHttpClient.h"

/**
 * @class CPlatformProtocol
 * @brief Centralizes platform MQTT topics and JSON envelope compatibility.
 */
class CPlatformProtocol
{
public:
    /**
     * @struct Command_S
     * @brief Parsed platform command envelope.
     */
    struct Command_S
    {
        std::string strCommand;
        std::string strRequestId;
        std::string strData{"{}"};
    };

    /**
     * @struct RegisterInput_S
     * @brief Input required to build one encrypted MQTT registration message.
     */
    struct RegisterInput_S
    {
        std::string strDeviceSn;
        std::string strUplinkType;
        std::string strUplinkInterface;
        std::string strLocalIp;
        std::string strStreamMode;
        std::string strRtspUrl;
        std::string strRtspAccount;
        std::string strRtspPassword;
        std::string strRtmpUrl;
        std::string strPublicKeyPath;
        std::string strPublicKeyId;
        std::int64_t llTimestampMs{0};
    };

    /**
     * @brief Returns the command topic for one device.
     * @author Codex
     * @param [IN] strDeviceSn Device serial number.
     * @return MQTT command topic.
     */
    static std::string BuildCommandTopic(const std::string &strDeviceSn);

    /**
     * @brief Returns the response topic for one device.
     * @author Codex
     * @param [IN] strDeviceSn Device serial number.
     * @return MQTT response topic.
     */
    static std::string BuildResponseTopic(const std::string &strDeviceSn);

    /**
     * @brief Returns the event topic for one device.
     * @author Codex
     * @param [IN] strDeviceSn Device serial number.
     * @return MQTT event topic.
     */
    static std::string BuildEventTopic(const std::string &strDeviceSn);

    /**
     * @brief Returns the status topic for one device.
     * @author Codex
     * @param [IN] strDeviceSn Device serial number.
     * @return MQTT status topic.
     */
    static std::string BuildStatusTopic(const std::string &strDeviceSn);

    /**
     * @brief Returns the registration topic for one device.
     * @author Codex
     * @param [IN] strDeviceSn Device serial number.
     * @return MQTT registration topic.
     */
    static std::string BuildRegisterTopic(const std::string &strDeviceSn);

    /**
     * @brief Parses a platform command envelope and preserves its Data JSON.
     * @author Codex
     * @param [IN] strPayload MQTT payload.
     * @param [OUT] stCommand Parsed command.
     * @param [OUT] strError Protocol failure description.
     * @return True when Command, RequestId and Data are valid.
     */
    static bool ParseCommand(const std::string &strPayload,
                             Command_S &stCommand,
                             std::string &strError);

    /**
     * @brief Builds a command response envelope.
     * @author Codex
     * @param [IN] strCommand Original command name.
     * @param [IN] strRequestId Original request identifier.
     * @param [IN] nReturn Device command return code.
     * @param [IN] strDataJson Response Data JSON.
     * @param [IN] strMessage Optional diagnostic message.
     * @return Compact JSON payload or an empty string on allocation failure.
     */
    static std::string BuildResponse(const std::string &strCommand,
                                     const std::string &strRequestId,
                                     int nReturn,
                                     const std::string &strDataJson,
                                     const std::string &strMessage);

    /**
     * @brief Builds a normal event envelope without changing Data field names.
     * @author Codex
     * @param [IN] strCommand Event command name.
     * @param [IN] strRequestId Event request identifier.
     * @param [IN] strDataJson Event Data JSON.
     * @return Compact JSON payload or an empty string on allocation failure.
     */
    static std::string BuildEvent(const std::string &strCommand,
                                  const std::string &strRequestId,
                                  const std::string &strDataJson);

    /**
     * @brief Builds an online, offline or LWT status envelope.
     * @author Codex
     * @param [IN] strDeviceSn Device serial number.
     * @param [IN] bOnline True for online and false for offline.
     * @param [IN] strReason Status transition reason.
     * @param [IN] llTimestampMs Unix timestamp in milliseconds, or zero for LWT.
     * @return Compact JSON payload or an empty string on allocation failure.
     */
    static std::string BuildStatus(const std::string &strDeviceSn,
                                   bool bOnline,
                                   const std::string &strReason,
                                   std::int64_t llTimestampMs);

    /**
     * @brief Builds the encrypted device-registration envelope.
     * @author Codex
     * @param [IN] stInput Registration input and credential material.
     * @param [OUT] strRequestId Generated registration request identifier.
     * @param [OUT] strPayload Compact registration JSON.
     * @param [OUT] strWarning Nonfatal credential encryption warning.
     * @return True when a registration payload is available.
     */
    static bool BuildRegistration(const RegisterInput_S &stInput,
                                  std::string &strRequestId,
                                  std::string &strPayload,
                                  std::string &strWarning);

    /**
     * @brief Builds the event-image upload result Data object.
     * @author Codex
     * @param [IN] stEvent Original event metadata.
     * @param [IN] strAlarmRequestId Original alarm request identifier.
     * @param [IN] stResponse HTTP image upload response.
     * @param [IN] bUploadOk Upload result.
     * @param [IN] strError Upload failure description.
     * @return Compact Data JSON or an empty string on allocation failure.
     */
    static std::string BuildImageUploadData(
        const NET_PlatformEventReport_S &stEvent,
        const std::string &strAlarmRequestId,
        const CPlatformHttpClient::EventImageResponse_S &stResponse,
        bool bUploadOk,
        const std::string &strError);

    /**
     * @brief Returns the current Unix timestamp in milliseconds.
     * @author Codex
     * @return Unix timestamp in milliseconds.
     */
    static std::int64_t GetCurrentTimestampMs();
};
