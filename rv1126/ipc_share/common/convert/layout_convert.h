/**
 * @FilePath     : layout_convert.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-16 17:00:39
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-06-16 17:05:06
 * @Description  : 布局定义装换
 */

#pragma once

#include <vector>

#include "Json.h"
#include "layout_define.h"

namespace Convert
{

    /**
     * @brief 转换函数
     * @param pRootJson json句柄
     * @param stInfo 位置信息
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object *pRootJson, Layout::Rect_S &stInfo, bool bOutStruct);

    /**
     * @brief 转换函数
     * @param pRootJson json句柄
     * @param stItem 通道号与预览窗口的对应信息
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object *pRootJson, Layout::Item_S &stItem, bool bOutStruct);

    /**
     * @brief 转换函数
     * @param pRootJson json句柄
     * @param vstItem 数组通道信息
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object *pRootJson, std::vector<Layout::Item_S> &vstItem, bool bOutStruct);

    /**
     * @brief 转换函数
     * @param pRootJson json句柄
     * @param stChnInfo 通道信息
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object *pRootJson, Layout::ChnInfo_S &stChnInfo, bool bOutStruct);

    /**
     * @brief 转换函数
     * @param pRootJson json句柄
     * @param vChnInfos 通道信息数组
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object *pRootJson, std::vector<Layout::ChnInfo_S> &vChnInfos, bool bOutStruct);

} // namespace Convert