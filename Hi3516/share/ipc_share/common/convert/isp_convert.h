/*** 
 * @FilePath     : isp_convert.h
 * @Author       : cyc
 * @Date         : 2025-06-13 10:47:22
 * @LastEditors  : cyc
 * @LastEditTime : 2025-08-13 17:36:10
 * @Description  : 图像配置数据结构转换
 */

#pragma once

#include <set>
#include "Json.h"
#include "isp_define.h"

namespace Convert
{
    /**
     * @brief  补光模式与Json报文的转换函数
     * @param  [Object] *pRootJson - json句柄
     * @param  [FillLight_S] &stInfo - 获取/设置的结构体的引用
     * @param  [bool] bOutStruct - true为转换成结构体，false:将结构体转化成json句柄
     * @return [*]
     * @author cyc
     * @note
     */
    void deal(Json::Object *pRootJson,ISP::Light_S &stInfo,bool bOutStruct);
    void deal(Json::Object *pRootJson,ISP::FillLight_S &stInfo,bool bOutStruct);

    /**
     * @brief  日夜切换与Json报文的转换函数
     * @param  [Object] *pRootJson - json句柄
     * @param  [FillLight_S] &stInfo - 获取/设置的结构体的引用
     * @param  [bool] bOutStruct - true为转换成结构体，false:将结构体转化成json句柄
     * @return [*]
     * @author cyc
     * @note
     */
    void deal(Json::Object *pRootJson,ISP::DayNightAttr_S &stInfo,bool bOutStruct);

    /**
     * @brief  图像参数与Json报文的转换函数
     * @param  [Object] *pRootJson - json句柄
     * @param  [FillLight_S] &stInfo - 获取/设置的结构体的引用
     * @param  [bool] bOutStruct - true为转换成结构体，false:将结构体转化成json句柄
     * @return [*]
     * @author cyc
     * @note
     */
    void deal(Json::Object *pRootJson,ISP::ImageParam_S &stInfo,bool bOutStruct);

    /**
     * @brief  视频调整参数与Json报文的转换函数
     * @param  [Object] *pRootJson - json句柄
     * @param  [FillLight_S] &stInfo - 获取/设置的结构体的引用
     * @param  [bool] bOutStruct - true为转换成结构体，false:将结构体转化成json句柄
     * @return [*]
     * @author cyc
     * @note
     */
    void deal(Json::Object *pRootJson,ISP::VideoAdjust_S &stInfo,bool bOutStruct);

    /**
     * @brief  白平衡参数与Json报文的转换函数
     * @param  [Object] *pRootJson - json句柄
     * @param  [FillLight_S] &stInfo - 获取/设置的结构体的引用
     * @param  [bool] bOutStruct - true为转换成结构体，false:将结构体转化成json句柄
     * @return [*]
     * @author cyc
     * @note
     */
    void deal(Json::Object *pRootJson,ISP::AwbAttr_S &stInfo,bool bOutStruct);

    /**
     * @brief  曝光参数与Json报文的转换函数
     * @param  [Object] *pRootJson - json句柄
     * @param  [FillLight_S] &stInfo - 获取/设置的结构体的引用
     * @param  [bool] bOutStruct - true为转换成结构体，false:将结构体转化成json句柄
     * @return [*]
     * @author cyc
     * @note
     */
    void deal(Json::Object *pRootJson,ISP::ExposureAttr_S &stInfo,bool bOutStruct);

    /**
     * @brief  宽动态参数与Json报文的转换函数
     * @param  [Object] *pRootJson - json句柄
     * @param  [FillLight_S] &stInfo - 获取/设置的结构体的引用
     * @param  [bool] bOutStruct - true为转换成结构体，false:将结构体转化成json句柄
     * @return [*]
     * @author cyc
     * @note
     */
    void deal(Json::Object *pRootJson,ISP::WdrAttr_S &stInfo,bool bOutStruct);

    /**
     * @brief  强光抑制参数与Json报文的转换函数
     * @param  [Object] *pRootJson - json句柄
     * @param  [FillLight_S] &stInfo - 获取/设置的结构体的引用
     * @param  [bool] bOutStruct - true为转换成结构体，false:将结构体转化成json句柄
     * @return [*]
     * @author cyc
     * @note
     */
    void deal(Json::Object *pRootJson,ISP::HlsAttr_S &stInfo,bool bOutStruct);

    /**
     * @brief  背光参数与Json报文的转换函数
     * @param  [Object] *pRootJson - json句柄
     * @param  [FillLight_S] &stInfo - 获取/设置的结构体的引用
     * @param  [bool] bOutStruct - true为转换成结构体，false:将结构体转化成json句柄
     * @return [*]
     * @author cyc
     * @note
     */
    void deal(Json::Object *pRootJson,ISP::BackLightArrt_S &stInfo,bool bOutStruct);

    /**
     * @brief  图像降噪参数与Json报文的转换函数
     * @param  [Object] *pRootJson - json句柄
     * @param  [FillLight_S] &stInfo - 获取/设置的结构体的引用
     * @param  [bool] bOutStruct - true为转换成结构体，false:将结构体转化成json句柄
     * @return [*]
     * @author cyc
     * @note
     */
    void deal(Json::Object *pRootJson,ISP::DnrAttr_S &stInfo,bool bOutStruct);

    /**
     * @brief  场景参数与Json报文的转换函数
     * @param  [Object] *pRootJson - json句柄
     * @param  [FillLight_S] &stInfo - 获取/设置的结构体的引用
     * @param  [bool] bOutStruct - true为转换成结构体，false:将结构体转化成json句柄
     * @return [*]
     * @author cyc
     * @note
     */
    void deal(Json::Object *pRootJson,ISP::SceneParams_S &stInfo,bool bOutStruct);
    void deal(Json::Object *pRootJson,std::vector<ISP::SceneParams_S> &stInfo,bool bOutStruct);
    void deal(Json::Object *pRootJson,ISP::AllSceneParams_S &stInfo,bool bOutStruct);

    /**
     * @brief  计划时间参数与Json报文的转换函数
     * @param  [Object] *pRootJson - json句柄
     * @param  [FillLight_S] &stInfo - 获取/设置的结构体的引用
     * @param  [bool] bOutStruct - true为转换成结构体，false:将结构体转化成json句柄
     * @return [*]
     * @author cyc  
     * @note
     */
    void deal(Json::Object *pRootJson,ISP::TimeRange_S &stInfo,bool bOutStruct);

    /**
     * @brief  计划配置参数与Json报文的转换函数
     * @param  [Object] *pRootJson - json句柄
     * @param  [FillLight_S] &stInfo - 获取/设置的结构体的引用
     * @param  [bool] bOutStruct - true为转换成结构体，false:将结构体转化成json句柄
     * @return [*]
     * @author cyc  
     * @note
     */
    void deal(Json::Object *pRootJson,ISP::SceneSchedule_S &stInfo,bool bOutStruct);
    void deal(Json::Object* pRootJson, ISP::SceneTime_S &stSceneTime, bool bOutStruct);
    void deal(Json::Object* pRootJson, ISP::MonthSchedule_S &stMonthSchedule, bool bOutStruct);
    void deal(Json::Object* pRootJson, std::vector<ISP::MonthSchedule_S> &monthSchedules, bool bOutStruct);

    /* 场景类型 */
    void deal(Json::Object* pRootJson, ISP::SceneType_E &stInfo, bool bOutStruct);


}; // namespace Convert