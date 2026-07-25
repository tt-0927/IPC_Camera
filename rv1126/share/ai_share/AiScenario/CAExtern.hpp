/*
 * @FilePath     : CAExtern.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-06-05 15:49:56
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-06-05 15:45:45
 * @Description  :
 */
#pragma once

#include "Extern.hpp"

namespace AiScenario_NS
{
    /* 音频数据 */
    typedef struct _CAData_
    {
        int8_t* pData;     /* 数据 */
        int     nDataSize; /* 数据大小 */
        int     nChannel;  /* 通道数 */
        int     nSample;   /* 采样率 */
        int     nDepth;    /* 位深 */

        void clear()
        {
            pData     = nullptr;
            nDataSize = 0;
            nChannel  = 0;
            nSample   = 0;
            nDepth    = 0;
        }

        _CAData_()
        {
            clear();
        }
    } CAData_S;

}    // namespace AiScenario_NS