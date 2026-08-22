/**
 * @file PlatformCommandRouter.h
 * @author Codex
 * @date 2026-08-22
 * @brief Declares MQTT command routing into the SDK configuration business layer.
 * @change 2026-08-22 Codex Initial implementation for platform migration.
 */
#pragma once

#include <functional>
#include <string>

class CPlatformHttpClient;

/**
 * @class CPlatformCommandRouter
 * @brief Resolves platform command names and executes the shared SDK business route.
 */
class CPlatformCommandRouter
{
public:
    /**
     * @struct Result_S
     * @brief Normalized MQTT command result.
     */
    struct Result_S
    {
        int nReturn{-1};
        int nCommand{0};
        std::string strData{"{}"};
        std::string strMessage;
    };

    using FallbackCommand_FN =
        std::function<int(const std::string &, const std::string &, std::string &)>;

    /**
     * @brief Configures image download and the optional host fallback executor.
     * @author Codex
     * @param [IN] pHttpClient Shared platform HTTP client.
     * @param [IN] strImageDownloadDirectory Directory for platform-provided NV21 files.
     * @param [IN] fnFallbackCommand Host fallback for commands absent from the SDK map.
     * @return True when required dependencies are valid.
     */
    bool Configure(CPlatformHttpClient *pHttpClient,
                   const std::string &strImageDownloadDirectory,
                   FallbackCommand_FN fnFallbackCommand);

    /**
     * @brief Executes one platform command synchronously on the command worker.
     * @author Codex
     * @param [IN] strCommand Platform command name or numeric SDK command code.
     * @param [IN] strData Command Data JSON.
     * @param [OUT] stResult Normalized return code and Data JSON.
     * @return True when a SDK or host route handles the command.
     */
    bool Execute(const std::string &strCommand,
                 const std::string &strData,
                 Result_S &stResult);

private:
    /**
     * @brief Downloads and validates face NV21 data before add or update operations.
     * @author Codex
     * @param [IN] nCommand Resolved SDK command code.
     * @param [INOUT] strData Command Data JSON updated with the local BinPath.
     * @param [OUT] strError Failure description.
     * @return True when no download is needed or the local file is valid.
     */
    bool PrepareFaceImage(int nCommand, std::string &strData, std::string &strError);

    CPlatformHttpClient *m_pHttpClient{nullptr};
    std::string m_strImageDownloadDirectory;
    FallbackCommand_FN m_fnFallbackCommand;
};
