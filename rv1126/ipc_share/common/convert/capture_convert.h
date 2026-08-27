/**
 * @FilePath     : capture_convert.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-07-15 16:19:31
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-15 16:28:12
 * @Description  : 抓图配置转换处理
 */

#pragma once

#include "Json.h"
#include "capture_define.h"

namespace Convert
{
    /* 抓图计划 */
    /**
     * @brief 转换时间段函数
     * @param pRootJson json句柄
     * @param stCaptureTime 设备信息数组
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object* pRootJson, Capture_NS::CaptureTime_S &stCaptureTime, bool bOutStruct);
    
    /**
     * @brief 转换一天计划函数
     * @param pRootJson json句柄
     * @param vstDaySchedules 设备信息数组
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object* pRootJson, Capture_NS::DaySchedule_S &vstDaySchedules, bool bOutStruct);

    /**
     * @brief 转换一周计划函数
     * @param pRootJson json句柄
     * @param stInfo 设备信息数组
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object *pRootJson, Capture_NS::CapturePlan_S &stInfo, bool bOutStruct);

    /* 时间间隔结构体 */
    void deal(Json::Object *pRootJson, Capture_NS::TimeInterval_S &stInfo, bool bOutStruct);
    /* 抓图定时/事件参数配置 */
    void deal(Json::Object *pRootJson, Capture_NS::CaptureConfig_S &stInfo, bool bOutStruct);
    /* 抓图参数 */
    void deal(Json::Object *pRootJson, Capture_NS::CaptureParam_S &stInfo, bool bOutStruct);
    /* 图片信息 */
    void deal(Json::Object* pRootJson, Capture_NS::CaptureInfo_S &stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, std::vector<Capture_NS::CaptureInfo_S> &Infos, bool bOutStruct);
}
