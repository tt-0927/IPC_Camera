/**
 * @file PlatformHttpClient.h
 * @author Codex
 * @date 2026-08-22
 * @brief Declares platform HTTP authentication, registration and image transfer.
 * @change 2026-08-22 Codex Initial implementation for platform migration.
 */
#pragma once

#include <cstdint>
#include <mutex>
#include <string>

#include "NetTVSDKServerInterface.h"

/**
 * @class CPlatformHttpClient
 * @brief Stateless-request HTTP client with a synchronized bearer token.
 */
class CPlatformHttpClient
{
public:
    /**
     * @struct Config_S
     * @brief Platform HTTP endpoint and credentials.
     */
    struct Config_S
    {
        std::string strHost;
        int nPort{0};
        std::string strUser;
        std::string strPassword;
    };

    /**
     * @struct EventImageRequest_S
     * @brief Event-image multipart upload parameters.
     */
    struct EventImageRequest_S
    {
        std::string strDeviceSn;
        int nEventType{0};
        std::string strEventName;
        int nChannel{0};
        std::int64_t llTimestampMs{0};
        std::string strRequestId;
        std::string strImagePath;
        std::string strFileName;
    };

    /**
     * @struct EventImageResponse_S
     * @brief Parsed event-image upload response.
     */
    struct EventImageResponse_S
    {
        int nStatusCode{0};
        std::string strStatus;
        std::string strMessage;
        std::string strImageUrl;
        std::string strImagePath;
        std::string strFileName;
    };

    /**
     * @brief Constructs an unconfigured HTTP client.
     * @author Codex
     */
    CPlatformHttpClient() = default;

    /**
     * @brief Copies a new HTTP endpoint and clears the old bearer token.
     * @author Codex
     * @param [IN] stConfig HTTP configuration.
     * @return True when the endpoint is valid.
     */
    bool Configure(const Config_S &stConfig);

    /**
     * @brief Authenticates with the platform and stores the bearer token.
     * @author Codex
     * @param [OUT] strError Non-sensitive failure description.
     * @return True when the platform returns a successful access token.
     */
    bool Login(std::string &strError);

    /**
     * @brief Registers the current device through the platform HTTP API.
     * @author Codex
     * @param [IN] stDevice Device identity profile.
     * @param [IN] strLiveUrl Current RTSP or RTMP live URL.
     * @param [IN] strProtocol Stream protocol name.
     * @param [OUT] strError Non-sensitive failure description.
     * @return True when the HTTP and business response are successful.
     */
    bool RegisterDevice(const NET_PlatformDeviceProfile_S &stDevice,
                        const std::string &strLiveUrl,
                        const std::string &strProtocol,
                        std::string &strError);

    /**
     * @brief Uploads one JPEG event image using multipart/form-data.
     * @author Codex
     * @param [IN] stRequest Upload request.
     * @param [OUT] stResponse Parsed platform response.
     * @param [OUT] strError Non-sensitive failure description.
     * @return True when the upload succeeds.
     */
    bool UploadEventImage(const EventImageRequest_S &stRequest,
                          EventImageResponse_S &stResponse,
                          std::string &strError);

    /**
     * @brief Downloads one HTTP resource to a local file using atomic replacement.
     * @author Codex
     * @param [IN] strUrl Absolute or platform-relative source URL.
     * @param [IN] strLocalPath Destination path.
     * @param [IN] llExpectedSize Expected byte size, or zero to skip validation.
     * @param [OUT] strError Non-sensitive failure description.
     * @return True when download, validation and replacement succeed.
     */
    bool DownloadFile(const std::string &strUrl,
                      const std::string &strLocalPath,
                      std::int64_t llExpectedSize,
                      std::string &strError) const;

    /**
     * @brief Clears the bearer token without changing endpoint configuration.
     * @author Codex
     * @return No return value.
     */
    void ClearSession();

    /**
     * @brief Returns whether a nonempty bearer token is available.
     * @author Codex
     * @return True after successful login and before session clearing.
     */
    bool IsAuthenticated() const;

private:
    /**
     * @brief Returns a synchronized copy of the current bearer token.
     * @author Codex
     * @return Bearer token copy.
     */
    std::string GetAccessToken() const;

    Config_S m_stConfig;
    mutable std::mutex m_mtxSession;
    std::string m_strAccessToken;
};
