/**
 * @file RecordFrameHub.h
 * @brief 录像帧流汇聚管理 - 管理 RecordFrame TCP 流的生命周期（启动/停止/清理）
 */
#pragma once

#include "ConfigQuery.h"
#include "BG6_ZHSJ/BU_SJCL/RecordFrameStream.h"

#include <map>
#include <memory>
#include <mutex>

/**
 * @brief 录像帧流管理单例
 * @details 维护 streamId → CRecordFrameStream 的映射，
 *          负责 HTTP 命令交互、TCP 连接建立与失败回滚
 */
class CRecordFrameHub : public CSingleton<CRecordFrameHub>
{
    CRecordFrameHub() {}
public:
    ~CRecordFrameHub() {}
    friend class CSingleton<CRecordFrameHub>;

    /**
     * @brief 启动录像帧流
     * @param lpUserID      用户登录句柄
     * @param pstCond       帧流请求条件
     * @param pstStreamInfo 输出：服务端返回的流信息
     * @param cbRecordFrame 帧数据回调
     * @param lpUserData    回调用户数据
     * @return TRUE 成功，FALSE 失败
     */
    BOOL StartStream(LPVOID lpUserID,
                     pNET_RecordFrameStreamCond_S pstCond,
                     pNET_RecordFrameStreamInfo_S pstStreamInfo,
                     NET_RecordFrameCallBack cbRecordFrame,
                     LPVOID lpUserData);

    /**
     * @brief 停止录像帧流
     * @param lpUserID   用户登录句柄
     * @param szStreamId 需要停止的流 ID
     * @return TRUE 成功，FALSE 失败
     */
    BOOL StopStream(LPVOID lpUserID, const CHAR* szStreamId);

    /**
     * @brief 清理所有活跃流（SDK 退出时调用）
     */
    void Cleanup();

private:
    std::map<std::string, std::shared_ptr<tvsdk::CRecordFrameStream>> m_streamMap;
    std::mutex m_mutex;
};
