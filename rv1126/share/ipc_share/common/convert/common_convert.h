/*** 
 * @FilePath     : common_convert.h
 * @Author       : 严泽辉 (yanzeh@kfb.cn)
 * @Date         : 2024-10-05 11:10:15
 * @LastEditors  : cyc
 * @LastEditTime : 2025-09-23 15:19:38
 * @Description  : 公共定义数据的转换
 */

#pragma once

#include "common_define.h"
#include "Json.h"
#include <vector>
namespace Convert
{
    /**
     * @brief 转换函数
     * @param pRootJson json句柄
     * @param stPageInfo 页数信息
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object *pRootJson, Common::PageInfo_S &stPageInfo, bool bOutStruct);

    /**
     * @brief  坐标数据转换函数
     * @param  [Object] *pRootJson - json句柄
     * @param  [Pos_S] &stInfo - 坐标数据(结构体)
     * @param  [bool] bOutStruct - 是否转换成结构体 false:将结构体转化成json句柄，true:相反
     * @return [*]
     * @author EasonLu
     * @note   包含点，点集，点浮点，点集浮点
     */
    void deal(Json::Object *pRootJson, Common::Pos_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, std::vector<Common::Pos_S> &vecInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Common::PosF_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, std::vector<Common::PosF_S> &vecInfo, bool bOutStruct);

    /**
     * @brief  矩形数据转换函数
     * @param  [Object] *pRootJson - json句柄
     * @param  [Rect_S] &stInfo - 矩形数据(结构体)
     * @param  [bool] bOutStruct - 是否转换成结构体 false:将结构体转化成json句柄，true:相反
     * @return [*]
     * @author EasonLu
     * @note   包含点，点集，点浮点，点集浮点
     */
    void deal(Json::Object *pRootJson, Common::Rect_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, std::vector<Common::Rect_S> &vecInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Common::RectF_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, std::vector<Common::RectF_S> &vecInfo, bool bOutStruct);

    /**
     * @brief  时间数据转换函数
     * @param  [Object] *pRootJson - json句柄
     * @param  [Time_S] &stInfo - 时间数据(结构体)
     * @param  [bool] bOutStruct - 是否转换成结构体 false:将结构体转化成json句柄，true:相反
     * @return [*]
     * @author EasonLu
     * @note   仅包含时间，不包含日期
     */
    void deal(Json::Object *pRootJson, Common::Time_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, std::vector<Common::Time_S> &vecInfo, bool bOutStruct);

    /**
     * @brief  时间段数据转换函数
     * @param  [Object] *pRootJson - json句柄
     * @param  [SchedTime_S] &stInfo - 时间段数据(结构体)
     * @param  [bool] bOutStruct - 是否转换成结构体 false:将结构体转化成json句柄，true:相反
     * @return [*]
     * @author EasonLu
     * @note   仅包含时间，不包含日期
     */
    void deal(Json::Object *pRootJson, Common::SchedTime_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, std::vector<Common::SchedTime_S> &vecInfo, bool bOutStruct);

     /**
     * @brief  日期数据转换函数
     * @param  [Object] *pRootJson - json句柄
     * @param  [Date_S] &stInfo - 日期数据(结构体)
     * @param  [bool] bOutStruct - 是否转换成结构体 false:将结构体转化成json句柄，true:相反
     * @return [*]
     * @author cyc
     * @note   包含年月日
     */
     void deal(Json::Object *pRootJson, Common::Date_S &stInfo, bool bOutStruct);
     void deal(Json::Object *pRootJson, std::vector<Common::Date_S> &vecInfo, bool bOutStruct);
} // namespace Convert