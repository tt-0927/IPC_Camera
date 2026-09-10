/**
 * @FilePath     : replay_convert.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-16 17:00:12
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-06-16 17:03:55
 * @Description  : 预览定义数据的转换
 */

#pragma once

#include "Json.h"
#include "replay_define.h"

namespace Convert
{
    /**
     * @brief 转换函数
     * @param pRootJson json句柄
     * @param stInfo 布局信息
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object* pRootJson, Replay::Info_S& stInfo, bool bOutStruct);

    /**
     * @brief 转换函数
     * @param pRootJson json句柄
     * @param stLayout 布局信息
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object* pRootJson, Replay::Layout_S& stLayout, bool bOutStruct);

    /**
     * @brief 转换函数
     * @param pRootJson json句柄
     * @param stInfo 轮巡信息
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object* pRootJson, Replay::Patrol_S& stInfo, bool bOutStruct);

    /**
     * @brief 转换函数
     * @param pRootJson json句柄
     * @param stInfo 回看信息
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object* pRootJson, Replay::Playback_S& stInfo, bool bOutStruct);

    /**
     * @brief 转换函数
     * @param pRootJson json句柄
     * @param stInfo 保存图片信息
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object* pRootJson, Replay::SaveImage_S& stInfo, bool bOutStruct);

    /**
     * @brief 转换函数
     * @param pRootJson json句柄
     * @param stInfo 电子放大信息
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object* pRootJson, Replay::DigitalZoom_S& stInfo, bool bOutStruct);

    /**
     * @brief 转换函数
     * @param pRootJson json句柄
     * @param stInfo 声音信息
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object* pRootJson, Replay::Voice_S& stInfo, bool bOutStruct);

    /**
     * @brief 转换函数
     * @param pRootJson json句柄
     * @param stInfo 播放性能信息
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object* pRootJson, Replay::PlaybackInfo_S& stInfo, bool bOutStruct);

    /**
     * @brief 转换函数
     * @param pRootJson json句柄
     * @param stInfo 码流信息
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object* pRootJson, Replay::StreamInfo_S& stInfo, bool bOutStruct);

    /**
     * @brief 转换函数
     * @param pRootJson json句柄
     * @param stInfo 展示信息
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object* pRootJson, Replay::AiShowInfo_S& stInfo, bool bOutStruct);

    /**
     * @brief 转换函数
     * @param pRootJson json句柄
     * @param stInfo 自适应分辨率信息
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object* pRootJson, Replay::AdaptiveResInfo_S& stInfo, bool bOutStruct);

    void deal(Json::Object* pRootJson, Replay::IpcList_S& stIpcList, bool bOutStruct);
    void deal(Json::Object *pRootJson, Replay::RecordTime_S &stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, Replay::ChnInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, std::vector<Replay::ChnInfo_S>& chnInfos, bool bOutStruct);

        
    /**
     * @brief 转换函数
     * @param pRootJson json句柄
     * @param stInfo 回放布局信息
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object* pRootJson, Replay::LayoutInfo_S& stInfo, bool bOutStruct);
	void deal(Json::Object* pRootJson, Replay::PlayInfo_S &stPlayInfo, bool bOutStruct);
	void deal(Json::Object* pRootJson, Replay::SeekInfo_S &stSeepInfo, bool bOutStruct);
	void deal(Json::Object* pRootJson, Replay::SpeedInfo_S &stSpeedInfo, bool bOutStruct);
	void deal(Json::Object* pRootJson, Replay::MediaInfo_S &stMediaInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, std::vector<Replay::MediaInfo_S> &mediaInfos, bool bOutStruct);
    void deal(Json::Object* pRootJson, Replay::LockInfo_S &stLockInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Replay::FileInfo_S &stFileInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, Replay::Stream::Info_S &stStreamInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Replay::Stream::ReplayRtpInfo_S &stRtpInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, Replay::Stream::Ctrl_S &stStreamCtrl, bool bOutStruct);
    void deal(Json::Object* pRootJson, Replay::Stream::MediaInfo_S &stStreamMediaInfo, bool bOutStruct);


}    // namespace Convert