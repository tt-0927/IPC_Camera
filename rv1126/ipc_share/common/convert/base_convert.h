/***
 * @FilePath     : base_convert.h
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2024-12-11 15:47:07
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-03-28 15:47:32
 * @Description  : 基本类型定义转换
 */

#pragma once
#include "base_define.h"
#include "Json.h"

namespace Convert
{
    /* deal函数声明宏定义：TypeName 类型名，Obj变量名 */
#define DEAL_DECLARATION(TypeName, Obj) \
    void deal(Json::Object *pRootJson, TypeName &Obj, bool bOutStruct);

    /* 类型，变量 */
    DEAL_DECLARATION(ChnId, chnId)
    DEAL_DECLARATION(Path, path)
    DEAL_DECLARATION(Password, password)
};