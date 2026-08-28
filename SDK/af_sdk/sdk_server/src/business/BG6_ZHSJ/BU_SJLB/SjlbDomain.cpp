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
    m_setTable[NET_CONTROL_RECORD]    = &CBujlbDomain::TemplatedSet<NET_RecordControlInfo_S>;
    m_setTable[NET_CONTROL_LIVE]      = &CBujlbDomain::TemplatedSet<NET_LiveStatusInfo_S>;
    m_setTable[NET_SET_DIRECTOR_MODE] = &CBujlbDomain::TemplatedSet<NET_DirectorModeInfo_S>;
    m_setTable[NET_CONTROL_CAMERA]    = &CBujlbDomain::TemplatedSet<NET_CameraControlInfo_S>;
    m_setTable[NET_CONTROL_PRESET_BIT] = &CBujlbDomain::TemplatedSet<NET_PresetBitCtrl_S>;
    m_setTable[NET_CONTROL_EXTERNAL]  = &CBujlbDomain::TemplatedSet<NET_ExternalControlInfo_S>;
    m_setTable[NET_CONTROL_LAYOUT]    = &CBujlbDomain::TemplatedSet<NET_LayoutSelfInfo_S>;
    m_setTable[NET_SET_PVW2PGM]       = &CBujlbDomain::TemplatedSet<NET_PVW2PGMInfo_S>;
    m_setTable[NET_ADD_APPOINTMENT]   = &CBujlbDomain::TemplatedSet<NET_AppointmentItem_S>;
    m_setTable[NET_CONTROL_REBOOT]    = &CBujlbDomain::TemplatedSet<NET_RebootInfo_S>;
    m_setTable[NET_SET_OUT_VOLUME]    = &CBujlbDomain::TemplatedSet<NET_OutVolume_S>;
    m_setTable[NET_SET_SSH_SAFE_INFO] = &CBujlbDomain::TemplatedSet<NET_SshSafeInfo_S>;
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
