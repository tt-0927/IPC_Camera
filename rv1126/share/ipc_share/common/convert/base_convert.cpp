/***
 * @FilePath     : base_convert.cpp
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2024-12-11 15:47:07
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-03-28 15:48:00
 * @Description  : 基本类型定义转换
 */

#include "base_convert.h"
#include "convert.h" /* 这个要放在xxxConvert.h的后面 */

/* deal函数实现：TypeName 类型名，Obj变量名 */
#define DEAL_IMPL(TypeName, Obj)                                                \
    void Convert::deal(Json::Object *pRootJson, TypeName &Obj, bool bOutStruct) \
    {                                                                           \
        if (!pRootJson)                                                         \
        {                                                                       \
            return;                                                             \
        }                                                                       \
        Convert::CConvert convert(bOutStruct);                                  \
        convert.field(pRootJson, #TypeName, Obj);                               \
    }

/* 类型，变量 */
DEAL_IMPL(ChnId, chnId)
DEAL_IMPL(Path, path)
DEAL_IMPL(Password, password)
