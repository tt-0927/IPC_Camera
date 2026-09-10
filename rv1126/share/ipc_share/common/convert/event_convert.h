/*** 
 * @FilePath     : event_convert.h
 * @Author       : huangjunda
 * @Date         : 2025-04-28 19:25:05
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-05-12 17:25:20
 * @Description  : 
 */

#pragma once

#include "Json.h"
#include "event_define.h"
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <list>
#include <set>

// namespace Convert
// {
    // void deal(Json::Object *pRootJson, RockitIvaPoint &stInfo, bool bOutStruct);
    // void deal(Json::Object* pRootJson, RockitIvaArea  &stInfo, bool bOutStruct);
    // void deal(Json::Object* pRootJson, RockitIvaAreas &stInfo, bool bOutStruct);
// }

namespace Convert
{
    
    void deal(Json::Object* pRootJson, Event::ChnSmart_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, std::set<Event::ChnSmart_S>& stInfo, bool bOutStruct);
    
    
    void deal(Json::Object* pRootJson, Event::NvrSmartInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, std::set<Event::NvrSmartInfo_S>& stInfo, bool bOutStruct);

    void deal(Json::Object* pRootJson, Event::AlgorithmConfig_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, std::vector<Event::AlgorithmConfig_S>& stInfo, bool bOutStruct);

    void deal(Json::Object *pRootJson, Event::OrdinaryEventEnableStatus_S &stInfo, bool bOutStruct);

    void deal(Json::Object *pRootJson, Event::SmartEventEnableStatus_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, std::vector<Event::Type_E> &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Event::SmartResourceAlloc_S &stInfo, bool bOutStruct);

    /* Metadata配置-Smart事件 */
    void deal(Json::Object *pRootJson, Event::MetadataSmart_S &stInfo, bool bOutStruct);
    /* Metadata配置-人脸抓拍 */
    void deal(Json::Object *pRootJson, Event::MetadataFaceCapture_S &stInfo, bool bOutStruct);
    /* Metadata配置 */
    void deal(Json::Object *pRootJson, Event::MetadataConfig_S &stInfo, bool bOutStruct);

    void deal(Json::Object* pRootJson, Event::BindVideo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, std::vector<Event::BindVideo_S>& stInfo, bool bOutStruct);
    /**
     * @brief 转换函数
     * @param pRootJson json句柄
     * @param stInfo 事件信息
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object* pRootJson, Event::Info_S& stInfo, bool bOutStruct);

    /**
     * @brief 转换函数
     * @param pRootJson json句柄
     * @param stCaptureInfo 抓拍事件信息
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object* pRootJson, Event::CaptureInfo_S& stCaptureInfo, bool bOutStruct);

    /**
     * @brief 转换函数
     * @param pRootJson json句柄
     * @param stCompareInfo 对比事件信息
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object* pRootJson, Event::CompareInfo_S& stCompareInfo, bool bOutStruct);

    /**
     * @brief 转换函数
     * @param pRootJson json句柄
     * @param stVehicleInfo 车辆事件
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object* pRootJson, Event::VehicleInfo_S& stVehicleInfo, bool bOutStruct);

    /**
     * @brief 转换函数
     * @param pRootJson json句柄
     * @param Infos 事件信息
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object* pRootJson, std::vector<Event::Info_S> &Infos, bool bOutStruct);

    /**
     * @brief 转换函数
     * @param pRootJson json句柄
     * @param VehicleInfos 车辆事件
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object* pRootJson, std::vector<Event::VehicleInfo_S> &VehicleInfos, bool bOutStruct);

    /**
     * @brief 转换函数
     * @param pRootJson json句柄
     * @param stRetrievalCond 搜索条件
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object* pRootJson, Event::RetrievalCond_S& stRetrievalCond, bool bOutStruct);
    void deal(Json::Object* pRootJson, Event::QuickEntry_S& stQuickEntry, bool bOutStruct);
    void deal(Json::Object* pRootJson, std::set<Event::QuickEntry_S>& stQuickEntry, bool bOutStruct);
    
    void deal(Json::Object* pRootJson, Event::Point_S& stPoint, bool bOutStruct);
    void deal(Json::Object* pRootJson, std::vector<Event::Point_S>& points, bool bOutStruct);
    /**
     * @brief 转换函数
     * @param pRootJson json句柄
     * @param stPoint 事件规则绘画
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object* pRootJson, Event::RuleInfo_S& stRuleInfo, bool bOutStruct);
    /**
     * @brief 转换函数
     * @param pRootJson json句柄
     * @param stPoint 事件规则绘画
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object* pRootJson, std::vector<Event::RuleInfo_S>& ruleInfos, bool bOutStruct);

	void deal(Json::Object* pRootJson, Event::FaceLibInfo_S &stTargetLibInfo, bool bOutStruct);
	void deal(Json::Object* pRootJson, std::vector<Event::FaceLibInfo_S> &targetLibInfos, bool bOutStruct);
	void deal(Json::Object* pRootJson, std::list<Event::FaceLibInfo_S> &targetLibInfos, bool bOutStruct);
	void deal(Json::Object* pRootJson, Event::FaceFind_S &stFaceFind, bool bOutStruct);
    void deal(Json::Object* pRootJson, Event::FaceIdInfo_S &stFaceIdInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, Event::AddFaceInfoResult &stAddFaceInfo, bool bOutStruct);
	void deal(Json::Object* pRootJson, Event::FaceInfo_S &stFaceInfo, bool bOutStruct);
	void deal(Json::Object* pRootJson, std::vector<Event::FaceInfo_S> &faceInfos, bool bOutStruct);
    void deal(Json::Object* pRootJson, Event::FaceCompareInfo_S &stFaceCompareInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, std::vector<Event::FaceCompareInfo_S> &faceCompareInfos, bool bOutStruct);
    void deal(Json::Object* pRootJson, Event::AlarmInfo_S &stAlarmInfo, bool bOutStruct);

    void deal(Json::Object* pRootJson, Event::AssociateLibInfo_S& stAssoFaceLibInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, std::vector<Event::AssociateLibInfo_S>& stAssoFaceLibInfos, bool bOutStruct);
    void deal(Json::Object* pRootJson, std::list<Event::AssociateLibInfo_S>& stAssoFaceLibInfos, bool bOutStruct);

    void deal(Json::Object* pRootJson, Event::TargetInfo_S &stTargetInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, std::vector<Event::TargetInfo_S> &stTargetInfos, bool bOutStruct);

}    // namespace Event
