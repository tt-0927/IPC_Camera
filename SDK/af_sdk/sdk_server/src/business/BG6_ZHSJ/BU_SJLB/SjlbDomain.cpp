/*
 * @FilePath     : sdk_new/sdk_server/src/business/BG6_ZHSJ/BU_SJLB/BujlbDomain.cpp
 * @Author       : chenchl
 * @Date         : 2026-08-22
 * @LastEditors  : chenchl
 * @LastEditTime : 2026-08-22
 * @Description  : BU_SJLB 配置域实现
 *                 注册录制/直播命令码→处理函数映射。
 */
#ifndef BU_SJLB_EXCLUDE

#include "SjlbDomain.h"
#include "SDKConvert.h"
#include "RecordInfoConvert.h"
#include "Json.h"
#include "NetSdkLog.h"

CBujlbDomain::CBujlbDomain()
{
    /* ==================================================================
     * Get 命令注册
     * ================================================================== */

    /* ===== 录制/直播状态查询 ===== */
    m_getTable[NET_GET_RECORD_INFO] = &CBujlbDomain::TemplatedGet<NET_RecordControlInfo_S>;
    m_getTable[NET_GET_LIVE_STATUS]   = &CBujlbDomain::TemplatedGet<NET_LiveStatusInfo_S>;
    /* ===== 录制文件列表查询（自定义处理，需要解析分页参数） ===== */
    m_getTable[NET_GET_RECORD_FILE_LIST] = &CBujlbDomain::HandleGetRecordFileList;
    m_getTable[NET_CONTROL_PRESET_BIT]   = &CBujlbDomain::TemplatedGet<NET_PresetBitInfo_S>;
    m_getTable[NET_CONTROL_LAYOUT]       = &CBujlbDomain::HandleGetLayout;

    m_getTable[NET_GET_APPOINTMENT_INFO]   = &CBujlbDomain::TemplatedGet<NET_AppointmentInfo_S>;
    m_getTable[NET_GET_OUT_VOLUME]         = &CBujlbDomain::TemplatedGet<NET_OutVolume_S>;
    m_getTable[NET_GET_SSH_SAFE_INFO]      = &CBujlbDomain::TemplatedGet<NET_SshSafeInfo_S>;

    /* ==================================================================
     * Set 命令注册
     * ================================================================== */

    /* ===== 录制/直播控制 ===== */
    m_setTable[NET_CONTROL_RECORD]    = &CBujlbDomain::HandleSetRecordControl;
    m_setTable[NET_CONTROL_LIVE]      = &CBujlbDomain::HandleSetLiveControl;

    m_setTable[NET_SET_DIRECTOR_MODE] = &CBujlbDomain::TemplatedSet<NET_DirectorModeInfo_S>;
    m_setTable[NET_CONTROL_CAMERA]    = &CBujlbDomain::TemplatedSet<NET_CameraControlInfo_S>;
    m_setTable[NET_CONTROL_PRESET_BIT] = &CBujlbDomain::TemplatedSet<NET_PresetBitCtrl_S>;
    m_setTable[NET_CONTROL_EXTERNAL]  = &CBujlbDomain::TemplatedSet<NET_ExternalControlInfo_S>;
    m_setTable[NET_CONTROL_LAYOUT]    = &CBujlbDomain::TemplatedSet<NET_LayoutSelfInfo_S>;
    m_setTable[NET_SET_PVW2PGM]       = &CBujlbDomain::TemplatedSet<NET_PVW2PGMInfo_S>;
    m_setTable[NET_ADD_APPOINTMENT]   = &CBujlbDomain::TemplatedSet<NET_AppointmentItem_S>;
    m_setTable[NET_SET_OUT_VOLUME]    = &CBujlbDomain::TemplatedSet<NET_OutVolume_S>;
    m_setTable[NET_SET_SSH_SAFE_INFO] = &CBujlbDomain::TemplatedSet<NET_SshSafeInfo_S>;
}

/* ===================== 录播自定义 Handler（1.可以使用域专用错误码描述 2.可以检查字段的缺少） ===================== */

/**
 * @brief 录制控制（522）
 * @details 手动实现（不复用 TemplatedSet），使用 get_recordErrMessage 提供录播专用错误码描述
 */
std::string CBujlbDomain::HandleSetRecordControl(INT32 nChannelId, INT32 nCommand,
                                                  const std::string& req_data,
                                                  const std::string& url_param)
{
    (void)url_param;

    if (req_data.empty())
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM, nCommand);
    }

    NET_RecordControlInfo_S stCfg;
    memset(&stCfg, 0, sizeof(stCfg));

    Json::Object* pRoot = Json::init(req_data);
    if (!pRoot)
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM, nCommand);
    }

    /* 前置校验：必填字段存在性（Status 拼错/缺失直接拒绝） */
    std::string strMissing = SDKConvert::check_requiredFields(pRoot, {"Status"});
    if (!strMissing.empty())
    {
        NETSDK_LOG_MESSAGE_WARN("SetRecordControl missing field [%s], cmd=%d",
                                strMissing.c_str(), nCommand);
        Json::deinit(pRoot);
        return SDKConvert::to_respString(NET_E_INVALID_PARAM, nCommand);
    }
    SDKConvert::deal(pRoot, stCfg, true);
    Json::deinit(pRoot);

    int nRespCode = executeSetDevConfigCb(nChannelId, nCommand, &stCfg);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_WARN("SetRecordControl callback failed, cmd=%d, ret=%d", nCommand, nRespCode);
    }

    /* 使用录播专用错误码描述 */
    std::string strMsg = SDKConvert::get_recordErrMessage(nRespCode);
    if (!strMsg.empty())
    {
        /* 录播业务错误码，内联构建响应JSON */
        Json::Object *pRoot = Json::init();
        Json::add(pRoot, NETSDK_JSON_DEVICE_NAME_KEY, g_sdkDeviceName);
        if (nCommand != 0) Json::add(pRoot, NETSDK_JSON_ACTIONCODE_KEY, nCommand);
        Json::add(pRoot, NETSDK_JSON_INNER_DATA_KEY, Json::init());
        Json::add(pRoot, NETSDK_JSON_RETURN_KEY, nRespCode);
        Json::add(pRoot, NETSDK_JSON_MESSAGE_KEY, strMsg);
        std::string data = Json::to_string(pRoot);
        Json::deinit(pRoot);
        return data;
    }
    return SDKConvert::to_respString((NET_COMMON_ECODE_E)nRespCode, nCommand);
}

/**
 * @brief 直播控制（524）
 * @details 手动实现（不复用 TemplatedSet），使用 get_recordErrMessage 提供录播专用错误码描述
 */
std::string CBujlbDomain::HandleSetLiveControl(INT32 nChannelId, INT32 nCommand,
                                                const std::string& req_data,
                                                const std::string& url_param)
{
    (void)url_param;

    if (req_data.empty())
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM, nCommand);
    }

    NET_LiveStatusInfo_S stCfg;
    memset(&stCfg, 0, sizeof(stCfg));

    Json::Object* pRoot = Json::init(req_data);
    if (!pRoot)
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM, nCommand);
    }

    SDKConvert::deal(pRoot, stCfg, true);
    Json::deinit(pRoot);

    int nRespCode = executeSetDevConfigCb(nChannelId, nCommand, &stCfg);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_WARN("SetLiveControl callback failed, cmd=%d, ret=%d", nCommand, nRespCode);
    }

    /* 使用录播专用错误码描述 */
    std::string strMsg = SDKConvert::get_recordErrMessage(nRespCode);
    if (!strMsg.empty())
    {
        /* 录播业务错误码，内联构建响应JSON */
        Json::Object *pRoot = Json::init();
        Json::add(pRoot, NETSDK_JSON_DEVICE_NAME_KEY, g_sdkDeviceName);
        if (nCommand != 0) Json::add(pRoot, NETSDK_JSON_ACTIONCODE_KEY, nCommand);
        Json::add(pRoot, NETSDK_JSON_INNER_DATA_KEY, Json::init());
        Json::add(pRoot, NETSDK_JSON_RETURN_KEY, nRespCode);
        Json::add(pRoot, NETSDK_JSON_MESSAGE_KEY, strMsg);
        std::string data = Json::to_string(pRoot);
        Json::deinit(pRoot);
        return data;
    }
    return SDKConvert::to_respString((NET_COMMON_ECODE_E)nRespCode, nCommand);
}

/**
 * @brief 判断命令码是否为 BU_SJLB 域的设备级命令（不需要通道号）
 * @details 仅收录录播独有命令；公用命令由 DeviceConfigBusiness 统一判断
 */
bool CBujlbDomain::IsDeviceLevelCommand(INT32 nCommand) const
{
    switch (nCommand)
    {
        case NET_CONTROL_RECORD:        /* 控制录制 */
        case NET_GET_RECORD_INFO:       /* 获取录制信息 */
        case NET_CONTROL_LIVE:          /* 控制直播 */
        case NET_GET_LIVE_STATUS:       /* 获取直播信息 */
        case NET_GET_RECORD_FILE_LIST:  /* 获取文件列表 */
        case NET_SET_DIRECTOR_MODE:     /* 控制导播模式 */
        case NET_CONTROL_CAMERA:        /* 云台控制 */
        case NET_CONTROL_PRESET_BIT:    /* 预置位控制 */
        case NET_CONTROL_EXTERNAL:      /* 调用中控 */
        case NET_CONTROL_LAYOUT:        /* 调用布局 */
        case NET_SET_PVW2PGM:           /* PVW切换到PGM */
        case NET_GET_APPOINTMENT_INFO:  /* 获取预约录制 */
        case NET_ADD_APPOINTMENT:       /* 设置预约录制 */
        case NET_GET_SSH_SAFE_INFO:     /* 获取ssh */
        case NET_SET_SSH_SAFE_INFO:     /* 设置ssh */
            return true;

        default:
            return false;
    }
}

/**
 * @brief 获取录制文件列表（自定义处理）
 * @details 从请求 JSON 中解析 CurPage/PageSize，
 *          调用设备回调填充文件列表，返回 JSON 响应。
 */
std::string CBujlbDomain::HandleGetRecordFileList(INT32 nChannelId, INT32 nCommand,
                                                  const std::string& req_data,
                                                  const std::string& url_param)
{
    (void)url_param;

    NET_RecordFileInfo_S stCfg;
    memset(&stCfg, 0, sizeof(NET_RecordFileInfo_S));

    /* 从请求 JSON 解析分页参数 */
    if (!req_data.empty())
    {
        Json::Object* pRoot = Json::init(req_data);
        if (pRoot)
        {
            Json::get(pRoot, "CurPage", stCfg.nCurPage);
            Json::get(pRoot, "PageSize", stCfg.nPageSize);
            Json::deinit(pRoot);
        }
    }

    /* 默认值 */
    if (stCfg.nCurPage <= 0) stCfg.nCurPage = 1;
    if (stCfg.nPageSize <= 0) stCfg.nPageSize = 10;

    NETSDK_LOG_MESSAGE_INFO("GetRecordFileList callback START, page=%d, size=%d",
                            stCfg.nCurPage, stCfg.nPageSize);

    int nRespCode = executeGetDevConfigCb(nChannelId, nCommand, &stCfg);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_WARN("GetRecordFileList callback failed, cmd=%d, ret=%d",
                                nCommand, nRespCode);
    }

    NETSDK_LOG_MESSAGE_INFO("GetRecordFileList callback cmd=%d, ret=%d, total=%d, count=%d",
                            nCommand, nRespCode, stCfg.nTotal, stCfg.nFileCount);

    return SDKConvert::to_respString(nRespCode, nCommand, nChannelId, stCfg);
}

/**
 * @brief 获取布局信息（自定义处理）
 * @details 从请求 JSON 中解析 MovieMode/MPlayout，
 *          调用设备回调填充布局数据，返回 JSON 响应。
 */
std::string CBujlbDomain::HandleGetLayout(INT32 nChannelId, INT32 nCommand,
                                          const std::string& req_data,
                                          const std::string& url_param)
{
    (void)url_param;

    NET_LayoutSelfInfo_S stCfg;
    memset(&stCfg, 0, sizeof(NET_LayoutSelfInfo_S));

    /* 从请求 JSON 解析布局模式与布局枚举 */
    if (!req_data.empty())
    {
        Json::Object* pRoot = Json::init(req_data);
        if (pRoot)
        {
            Json::get(pRoot, "MovieMode", stCfg.nMovieMode);
            Json::get(pRoot, "MPlayout",  stCfg.nMplayout);
            Json::deinit(pRoot);
        }
    }

    /* 默认值：PGM 模式 */
    if (stCfg.nMovieMode < 0) stCfg.nMovieMode = 0;
    if (stCfg.nMplayout < 0)  stCfg.nMplayout  = 0;

    NETSDK_LOG_MESSAGE_INFO("GetLayout callback START, mode=%d, layout=%d",
                            stCfg.nMovieMode, stCfg.nMplayout);

    int nRespCode = executeGetDevConfigCb(nChannelId, nCommand, &stCfg);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_WARN("GetLayout callback failed, cmd=%d, ret=%d",
                                nCommand, nRespCode);
    }

    NETSDK_LOG_MESSAGE_INFO("GetLayout callback cmd=%d, ret=%d, num=%d",
                            nCommand, nRespCode, stCfg.nNum);

    return SDKConvert::to_respString(nRespCode, nCommand, nChannelId, stCfg);
}

#endif /* BU_SJLB_EXCLUDE */
