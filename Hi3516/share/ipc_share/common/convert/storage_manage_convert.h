/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-08-11 10:47:06
 * @LastEditors: 梁浩尧 lianghaoyao@kfb.cn
 * @LastEditTime: 2025-12-23 16:56:45
 * @FilePath: /hisi/share/ipc_share/common/convert/storage_convert.h
 * @Description: 存储管理数据的转换
 */
#pragma once

#include "Json.h"
#include "storage_manage_define.h"

namespace Convert
{
    /**
     * @brief 转换存储管理参数函数
     * @param pRootJson json句柄
     * @param StorageManageParam 存储管理参数
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */
    void deal(Json::Object* pRootJson, StorageManage_NS::StorageManage_S &StorageManageParam, bool bOutStruct);

    /**
     * @brief 转换存储目录信息函数
     * @param pRootJson json句柄
     * @param stDirInfo 存储目录信息参数
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */    
    void deal(Json::Object* pRootJson, StorageManage_NS::DirInfo_S &stDirInfo, bool bOutStruct);

    /**
     * @brief 转换是否格式化函数
     * @param pRootJson json句柄
     * @param bIsInitSdCard 是否格式化sd卡
     * @param bOutStruct 是否转换成结构体，false:将结构体转化成json句柄，true:相反
     */    
    void deal(Json::Object* pRootJson, bool &bIsInitSdCard, bool bOutStruct);

}