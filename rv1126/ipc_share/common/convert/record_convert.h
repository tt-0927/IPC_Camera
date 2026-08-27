/**
 * @FilePath     : record_convert.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-16 16:57:26
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-01-07 14:36:32
 * @Description  : 录制配置转换处理
 */

#pragma once

#include <vector>
#include <set>
#include "Json.h"
#include "record_define.h"

namespace Convert
{

    /**
     * @brief 转换函数
     * @param pRootJson json句柄
     * @param stRecordInfo 设备信息
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object *pRootJson, Record_NS::Info_S &stRecordInfo, bool bOutStruct);

    /**
     * @brief 转换函数
     * @param pRootJson json句柄
     * @param vecRecordInfos 设备信息数组
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object *pRootJson, std::vector<Record_NS::Info_S> &vecRecordInfos, bool bOutStruct);

	void deal(Json::Object* pRootJson, Record_NS::VideoConfigInfo_S &stVideoConfigInfo, bool bOutStruct);
	void deal(Json::Object* pRootJson, Record_NS::AudioConfigInfo_S &stAudioConfigInfo, bool bOutStruct);
	void deal(Json::Object* pRootJson, Record_NS::MediaData_S &stMediaData, bool bOutStruct);
	void deal(Json::Object* pRootJson, Record_NS::FileInfo_S &stFileInfo, bool bOutStruct);
	void deal(Json::Object* pRootJson, std::vector<Record_NS::FileInfo_S>  &infos, bool bOutStruct);
	void deal(Json::Object* pRootJson, Record_NS::TsFileInfo_S &stTsFileInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, std::vector<Record_NS::TsFileInfo_S> &stTsFileInfo, bool bOutStruct);
    
	void deal(Json::Object* pRootJson, Record_NS::Find_S &stFind, bool bOutStruct);
    void deal(Json::Object* pRootJson, Record_NS::TsFind_S &stFind, bool bOutStruct);

    void deal(Json::Object* pRootJson, Record_NS::RetrievalCond_S& stRetrievalCond, bool bOutStruct);
	void deal(Json::Object* pRootJson, Record_NS::VideoTime_S &stVideoTime, bool bOutStruct);
	void deal(Json::Object* pRootJson, Record_NS::FindResult_S &stFindResult, bool bOutStruct);
	void deal(Json::Object* pRootJson, std::vector<Record_NS::FindResult_S> &findResults, bool bOutStruct);
	void deal(Json::Object* pRootJson, Record_NS::UpdateInfo_S &stUpdateInfo, bool bOutStruct);

    void deal(Json::Object* pRootJson, Record_NS::RecordTime_S &stRecordTime, bool bOutStruct);
	void deal(Json::Object* pRootJson, Record_NS::DaySchedule_S &stDaySchedule, bool bOutStruct);
	void deal(Json::Object* pRootJson, std::vector<Record_NS::DaySchedule_S> &daySchedules, bool bOutStruct);
	void deal(Json::Object* pRootJson, Record_NS::Schedule_S &stSchedule, bool bOutStruct);
	void deal(Json::Object* pRootJson, Record_NS::AdvancedParam_S &stAdvancedParam, bool bOutStruct);
    void deal(Json::Object* pRootJson, std::set<Record_NS::AdvancedParam_S> &stAdvancedParam, bool bOutStruct);

    void deal(Json::Object* pRootJson, Record_NS::OtherInfo_S &stOtherInfo, bool bOutStruct);

    void deal(Json::Object* pRootJson, Record_NS::RecordStatusInfo_S &stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, std::vector<Record_NS::RecordStatusInfo_S> &vecInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, Record_NS::DownloadInfo_S &stDownloadInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, std::vector<Record_NS::DownloadInfo_S> &downloadInfo, bool bOutStruct);
    
    void deal(Json::Object* pRootJson, Record_NS::DownloadInfo_S &stDownloadInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, std::vector<Record_NS::DownloadInfo_S> &downloadInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, Record_NS::DownloadProgress_S &stDownloadProgress, bool bOutStruct);
    void deal(Json::Object* pRootJson, std::vector<Record_NS::DownloadProgress_S> &downloadProgress, bool bOutStruct);
}
