/*
 * @FilePath     : ParseDataExtern.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-04-01 15:14:58
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-04-01 15:18:56
 * @Description  :
 */
#pragma once

namespace ParseData_NS
{
    /* 处理类型 */
    typedef enum _Type_
    {
        PARSE_JSON = 0, /* 解析Json数据 */
    } Type_E;

    /* 必需参数 */
    typedef struct _NeedParam_
    {
        Type_E enType; /* 使用的类型 */

        _NeedParam_()
        {
            enType = PARSE_JSON;
        }

        void clear()
        {
            enType = PARSE_JSON;
        }
    } NeedParam_S;

    /* 额外参数 */
    typedef struct _ExParam_
    {
        bool Reserved; /* 预留位，没用 */

        _ExParam_()
        {
            Reserved = false;
        }

        void clear()
        {
            Reserved = false;
        }

    } ExParam_S;

    /* 参数结构体 */
    typedef struct _InParam_
    {
        NeedParam_S stNeedParam; /* 必需参数 */
        ExParam_S   stExParam;   /* 额外参数 */

        void clear()
        {
            stNeedParam.clear();
            stExParam.clear();
        }
    } InParam_S;

}    // namespace ParseData_NS