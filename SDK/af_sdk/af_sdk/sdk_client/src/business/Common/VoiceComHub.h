/**
 * @file VoiceComHub.h
 * @brief 语音对讲汇聚管理 - 管理 VoiceCom TCP 连接的生命周期与音频参数校验
 */
#pragma once

#include "ConfigQuery.h"
#include "BG6_ZHSJ/BU_SJCL/VoiceComStream.h"

#include <map>
#include <memory>
#include <mutex>

/**
 * @brief 语音对讲管理单例
 * @details 维护 userID → CVoiceComStream 的映射，
 *          负责音频参数校验、UDP 连接建立与数据收发
 */
class CVoiceComHub : public CSingleton<CVoiceComHub>
{
    CVoiceComHub() {}
public:
    ~CVoiceComHub() {}
    friend class CSingleton<CVoiceComHub>;

    /**
     * @brief 启动语音对讲
     * @param lpUserID      用户登录句柄
     * @param pstStartInfo  对讲启动参数（含音频端口、音频参数）
     * @param cbVoiceCom    音频数据回调
     * @param lpUserData    回调用户数据
     * @return TRUE 成功，FALSE 失败
     */
    BOOL Start(LPVOID lpUserID,
               pNET_VoiceComStartInfo_S pstStartInfo,
               NET_VoiceComCallBack cbVoiceCom,
               LPVOID lpUserData);

    /**
     * @brief 发送音频数据到设备
     * @param lpUserID 用户登录句柄
     * @param pData    音频帧数据
     * @param dwSize   数据长度（字节）
     * @return TRUE 成功，FALSE 失败
     */
    BOOL SendData(LPVOID lpUserID, const CHAR* pData, UINT32 dwSize);

    /**
     * @brief 停止语音对讲
     * @param lpUserID 用户登录句柄
     * @return TRUE 成功，FALSE 失败
     */
    BOOL Stop(LPVOID lpUserID);

    /**
     * @brief 清理所有活跃对讲（SDK 退出时调用）
     */
    void Cleanup();

private:
    /**
     * @brief 音频参数校验与规范化
     * @param audioParam 音频参数，会被就地修改
     * @return 校验通过返回 true，失败返回 false
     */
    static bool NormalizeAudioParam(NET_VoiceComAudioParam_S& audioParam);

    std::map<LPVOID, std::shared_ptr<tvsdk::CVoiceComStream>> m_comMap;
    std::mutex m_mutex;
};
