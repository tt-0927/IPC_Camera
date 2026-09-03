/*
 * @FilePath     : sdk_new/sdk_server/src/cb/BG6_ZHSJ/BU_SJCL/NetTVNvrConfigCb.c
 * @Author       : ITC
 * @Date         : 2026-08-19
 * @LastEditors  : ITC
 * @LastEditTime : 2026-08-19
 * @Description  : NVR独有配置回调注册与执行实现（BG6_ZHSJ/BU_SJCL部门专用）
 *                 包含NVR特有功能：
 *                 1. RTSP流地址获取（GetRtspUrl）
 *                 2. 回放URL获取（GetReplayUrl）
 *                 3. 回放控制（ControlReplay：播放/暂停/快进/快退/定位）
 *                 4. 回放录像列表获取（GetReplayRecordList）
 *                 这些功能为NVR特有，不属于通用设备配置，从Common/NetTVConfigCb.c迁移至此。
 *                 降级策略：专用回调未注册时，走Common的命令码分发（executeGetDevConfigCb/SetDevConfig）。
 */

#include <stdio.h>
#include <stddef.h>
#include "NetTVNvrConfigCbExecute.h"
#include "NetTVConfigCbExecute.h"
#include "NetTVSDKServerInterface.h"

/**
 * @brief NVR独有配置回调表（独立于Common通用配置回调表）
 * @note RTSP和回放为NVR特有功能，不放入Common的g_stConfigCbTable
 */
typedef struct tagNETTVNvrCbTable
{
    NET_CB_GetRtspUrl          cbGetRtspUrl;            /* RTSP流地址获取回调 */
    NET_CB_GetReplayUrl        cbGetReplayUrl;          /* 回放URL获取回调 */
    NET_CB_ControlReplay       cbControlReplay;         /* 回放控制回调 */
    NET_CB_GetReplayRecordList cbGetReplayRecordList;   /* 回放录像列表获取回调 */
} NET_NVR_CB_TABLE_S;

static NET_NVR_CB_TABLE_S g_stNvrCbTable = {0};  /* NVR独有配置回调表实例 */

/* ===================== RTSP流地址配置 ===================== */

/**
 * @brief 注册获取RTSP实时预览地址的回调函数
 * @param [in] pCb 用于填充 RTSP URL 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetRtspUrlCb(NET_CB_GetRtspUrl pCb)
{
    if (pCb == NULL)
    {
        fprintf(stderr, "[DIAG-RTSP-SDK] register callback rejected: null\n");
        fflush(stderr);
        return FALSE;
    }

    if (g_stNvrCbTable.cbGetRtspUrl != NULL)
    {
        fprintf(stderr, "[DIAG-RTSP-SDK] register callback rejected: already registered\n");
        fflush(stderr);
        return FALSE;
    }

    g_stNvrCbTable.cbGetRtspUrl = pCb;
    fprintf(stderr, "[DIAG-RTSP-SDK] register callback succeeded\n");
    fflush(stderr);
    return TRUE;
}

/**
 * @brief 执行RTSP流地址获取回调
 * @param [IN] dwChannelID 通道号
 * @param [OUT] pInfo RTSP URL返回信息
 * @return NET_E_SUCCEED 成功，其他值失败
 * @note 回调执行优先级：专用RTSP回调 > 通用配置回调（降级走Common命令码分发）
 */
int executeGetRtspUrlCb(INT32 dwChannelID, pNET_RtspUrlInfo_S pInfo)
{
    fprintf(stderr, "[DIAG-RTSP-SDK] executeGetRtspUrlCb enter: channel=%d info=%p\n",
            dwChannelID, (void *)pInfo);
    fflush(stderr);
    if (pInfo == NULL)
    {
        fprintf(stderr, "[DIAG-RTSP-SDK] executeGetRtspUrlCb invalid info\n");
        fflush(stderr);
        return NET_E_INVALID_PARAM;
    }

    /* 优先使用专用RTSP回调 */
    if (g_stNvrCbTable.cbGetRtspUrl != NULL)
    {
        fprintf(stderr, "[DIAG-RTSP-SDK] invoking registered callback\n");
        fflush(stderr);
        int nRet = g_stNvrCbTable.cbGetRtspUrl(dwChannelID, pInfo);
        fprintf(stderr, "[DIAG-RTSP-SDK] registered callback returned: ret=%d\n", nRet);
        fflush(stderr);
        return nRet;
    }

    fprintf(stderr, "[DIAG-RTSP-SDK] callback not registered, fallback command=%d\n",
            NET_GET_RTSPURLCFG);
    fflush(stderr);
    /* 降级到通用配置回调（走Common的命令码分发） */
    return executeGetDevConfigCb(dwChannelID, NET_GET_RTSPURLCFG, pInfo);
}

/* ===================== 回放URL配置 ===================== */

/**
 * @brief 注册获取回放地址的回调函数
 * @param [in] pCb 用于填充回放 URL 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetReplayUrlCb(NET_CB_GetReplayUrl pCb)
{
    if (pCb == NULL)
    {
        return FALSE;
    }

    if (g_stNvrCbTable.cbGetReplayUrl != NULL)
    {
        return FALSE;
    }

    g_stNvrCbTable.cbGetReplayUrl = pCb;
    return TRUE;
}

/**
 * @brief 执行回放URL获取回调
 * @param [INOUT] pInfo 回放查询条件和播放URL返回信息
 * @return NET_E_SUCCEED 成功，其他值失败
 * @note 回调执行优先级：专用回放URL回调 > 通用配置回调（降级走Common命令码分发）
 */
int executeGetReplayUrlCb(pNET_ReplayUrlInfo_S pInfo)
{
    if (pInfo == NULL)
    {
        return NET_E_INVALID_PARAM;
    }

    /* 通道号兜底：若未指定通道号，无法降级到命令码分发 */
    INT32 dwChannelID = pInfo->uChannel;

    /* 优先使用专用回放URL回调 */
    if (g_stNvrCbTable.cbGetReplayUrl != NULL)
    {
        return g_stNvrCbTable.cbGetReplayUrl(pInfo);
    }

    /* 降级到通用配置回调（走Common的命令码分发） */
    return executeGetDevConfigCb(dwChannelID, NET_GET_REPLAY_URLCFG, pInfo);
}

/* ===================== 回放控制配置 ===================== */

/**
 * @brief 注册回放控制回调函数（播放/暂停/快进/快退/定位）
 * @param [in] pCb 用于执行回放控制命令的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterControlReplayCb(NET_CB_ControlReplay pCb)
{
    if (pCb == NULL)
    {
        return FALSE;
    }

    if (g_stNvrCbTable.cbControlReplay != NULL)
    {
        return FALSE;
    }

    g_stNvrCbTable.cbControlReplay = pCb;
    return TRUE;
}

/**
 * @brief 执行回放控制回调
 * @param [INOUT] pInfo 回放控制输入输出参数
 * @return NET_E_SUCCEED 成功，其他值失败
 * @note 回调执行优先级：专用回放控制回调 > 通用配置设置回调（降级走Common命令码分发）
 */
int executeControlReplayCb(pNET_ReplayCtrlInfo_S pInfo)
{
    if (pInfo == NULL)
    {
        return NET_E_INVALID_PARAM;
    }

    printf("[NetTVNvrConfigCb] ControlReplay callback: channel=%d, ctrlType=%d, startTime=[%s], endTime=[%s], sessionId=[%s]\n",
           pInfo->uChannel, pInfo->uCtrlType, pInfo->szStartTime, pInfo->szEndTime, pInfo->szSessionId);

    /* 通道号兜底 */
    INT32 dwChannelID = pInfo->uChannel;

    /* 优先使用专用回放控制回调 */
    if (g_stNvrCbTable.cbControlReplay != NULL)
    {
        return g_stNvrCbTable.cbControlReplay(pInfo);
    }

    /* 降级到通用配置设置回调（走Common的命令码分发） */
    return executeSetDevConfigCb(dwChannelID, NET_SET_REPLAY_CTRL, pInfo);
}

/* ===================== 回放录像列表配置 ===================== */

/**
 * @brief 注册获取回放录像文件列表的回调函数
 * @param [in] pCb 用于填充录像文件列表的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetReplayRecordListCb(NET_CB_GetReplayRecordList pCb)
{
    if (pCb == NULL)
    {
        return FALSE;
    }

    if (g_stNvrCbTable.cbGetReplayRecordList != NULL)
    {
        return FALSE;
    }

    g_stNvrCbTable.cbGetReplayRecordList = pCb;
    return TRUE;
}

/**
 * @brief 执行回放录像列表获取回调
 * @param [INOUT] pInfo 查询条件及结果
 * @return NET_E_SUCCEED 成功，其他值失败
 * @note 回调执行优先级：专用回放录像列表回调 > 通用配置回调（降级走Common命令码分发）
 */
int executeGetReplayRecordListCb(pNET_ReplayRecordList_S pInfo)
{
    if (pInfo == NULL)
    {
        return NET_E_INVALID_PARAM;
    }

    /* 通道号兜底 */
    INT32 dwChannelID = pInfo->uChannel;

    /* 优先使用专用回放录像列表回调 */
    if (g_stNvrCbTable.cbGetReplayRecordList != NULL)
    {
        return g_stNvrCbTable.cbGetReplayRecordList(pInfo);
    }

    /* 降级到通用配置回调（走Common的命令码分发） */
    return executeGetDevConfigCb(dwChannelID, NET_GET_REPLAY_RECORD_LIST, pInfo);
}

/* ===================== 录像计划与高级参数（NVR独有） ===================== */

/**
 * @brief 注册获取录像计划配置的回调函数
 * @param [in] pCb 用于填充 NET_RecordSchedule_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note NVR独有：按通道配置的周录像计划（NVR录像业务调度）
 */
NET_API BOOL STDCALL NET_serverRegisterGetRecordScheduleCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_RECORD_SCHEDULE, pCb);
}

/**
 * @brief 注册设置录像计划配置的回调函数
 * @param [in] pCb 用于读取 NET_RecordSchedule_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note NVR独有：按通道配置的周录像计划（NVR录像业务调度）
 */
NET_API BOOL STDCALL NET_serverRegisterSetRecordScheduleCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_RECORD_SCHEDULE, pCb);
}

/**
 * @brief 注册获取录像高级参数的回调函数
 * @param [in] pCb 用于填充 NET_RecordAdvancedParam_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note NVR独有：预录/延时录/码流类型等高级参数
 */
NET_API BOOL STDCALL NET_serverRegisterGetRecordAdvancedParamCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_RECORD_ADVANCED_PARAM, pCb);
}

/**
 * @brief 注册设置录像高级参数的回调函数
 * @param [in] pCb 用于读取 NET_RecordAdvancedParam_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note NVR独有：预录/延时录/码流类型等高级参数
 */
NET_API BOOL STDCALL NET_serverRegisterSetRecordAdvancedParamCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_RECORD_ADVANCED_PARAM, pCb);
}

/**
 * @brief 注册查找录像文件信息的回调函数
 * @param [in] pCb 用于按条件查询录像文件列表的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note NVR独有：检索NVR本地录像库的文件列表
 */
NET_API BOOL STDCALL NET_serverRegisterFindRecordFileInfoCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_FIND_RECORD_FILE_INFO, pCb);
}

/**
 * @brief 注册下载录像文件的回调函数
 * @param [in] pCb 用于执行录像文件下载的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note NVR独有：从NVR录像库下载指定录像文件
 */
NET_API BOOL STDCALL NET_serverRegisterDownloadRecordFileCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_DOWNLOAD_RECORD_FILE, pCb);
}

/* ===================== 预览信息（NVR独有） ===================== */

/**
 * @brief 注册获取预览信息的回调函数
 * @param [in] pCb 用于填充 NET_PreviewInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note NVR独有：NVR向客户端预分配的预览通道参数
 */
NET_API BOOL STDCALL NET_serverRegisterGetPreviewInfoCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_PREVIEW_INFO, pCb);
}

/**
 * @brief 注册设置预览信息的回调函数
 * @param [in] pCb 用于读取 NET_PreviewInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note NVR独有：NVR向客户端预分配的预览通道参数
 */
NET_API BOOL STDCALL NET_serverRegisterSetPreviewInfoCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_PREVIEW_INFO, pCb);
}

/* ===================== 对讲控制（NVR独有） ===================== */

/**
 * @brief 注册设置对讲状态的回调函数（开启/关闭对讲）
 * @param [in] pCb 用于执行对讲启停控制的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note NVR独有：NVR转发对讲音频到IPC的中枢控制
 */
NET_API BOOL STDCALL NET_serverRegisterSetTalkbackStateCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_STATE_TALKBACK, pCb);
}

/**
 * @brief 注册设置对讲音频流向（发送至码流）的回调函数
 * @param [in] pCb 用于将本地音频注入码流的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note NVR独有：NVR将对讲音频注入码流输出
 */
NET_API BOOL STDCALL NET_serverRegisterSetTalkbackToStreamCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_TO_STREAM_TALKBACK, pCb);
}

/**
 * @brief 注册获取对讲音频流向（从码流读取）的回调函数
 * @param [in] pCb 用于从码流提取对讲音频的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note NVR独有：NVR从码流提取对讲音频分发
 */
NET_API BOOL STDCALL NET_serverRegisterGetTalkbackFromStreamCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_FROM_STREAM_TALKBACK, pCb);
}

/**
 * @brief 注册回放对讲控制的回调函数
 * @param [in] pCb 用于回放期间对讲控制的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note NVR独有：NVR回放录像时的对讲音频控制
 */
NET_API BOOL STDCALL NET_serverRegisterSetReplayTalkbackCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_REPLAY_TALKBACK, pCb);
}

/* ===================== 通道信息（NVR独有） ===================== */

/**
 * @brief 注册获取通道信息的回调函数
 * @param [in] pCb 用于填充 NET_ChannelInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note NVR独有：NVR通道管理（IPC无通道概念）
 */
NET_API BOOL STDCALL NET_serverRegisterGetChannelInfoCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_CHANNEL_INFO, pCb);
}

/**
 * @brief 注册录像控制回调函数（启停录像）
 * @param [in] pCb 用于执行录像启停控制的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterControlRecordInfoCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_CONTROL_RECORD_INFO, pCb);
}

/**
 * @brief 注册获取录像状态的回调函数
 * @param [in] pCb 用于填充 NET_RecordStatus_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetRecordStatusCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_RECORD_STATUS, pCb);
}
