/*
 * @FilePath     : PerfManageExtern.hpp
 * @Author       : 李辉 lih@kfb.cn
 * @Date         : 2024-03-28 10:02:08
 * @LastEditors  : 李辉 lih@kfb.cn
 * @LastEditTime : 2024-03-28 10:02:08
 * @Description  :
 */
#pragma once

#include <iostream>

namespace Ai0630_NS
{
    struct CpuInfo_S
    {
        double dHz;      /* 主频 */
        int    nTemp;    /* 温度 */
        double dPercent; /* 利用率 */
        int    nProcess; /* 进程数 */
        double dSpeed;   /* 速度 */

        void clear()
        {
            dHz      = 0.0;
            nTemp    = 0;
            dPercent = 0.0;
            nProcess = 0;
            dSpeed   = 0.0;
        }

        void print() const
        {
            std::cout << "\n获取CPU信息请求:=============" << std::endl;
            std::cout << "主频:" << dHz << std::endl;
            std::cout << "温度:" << nTemp << std::endl;
            std::cout << "利用率:" << dPercent << std::endl;
            std::cout << "进程数:" << nProcess << std::endl;
            std::cout << "速度:" << dSpeed << std::endl;
            std::cout << "end:=============" << std::endl;
        }
    };

    struct MemoryInfo_S
    {
        double dUsed;   /* 已用 */
        double dTotal;  /* 总共 */
        double dUsable; /* 可用 */

        void clear()
        {
            dUsed   = 0.0;
            dTotal  = 0.0;
            dUsable = 0.0;
        }

        void print() const
        {
            std::cout << "\n获取存储信息请求:=============" << std::endl;
            std::cout << "已用:" << dUsed << std::endl;
            std::cout << "总共:" << dTotal << std::endl;
            std::cout << "可用:" << dUsable << std::endl;
            std::cout << "end:=============" << std::endl;
        }
    };

    struct GpuInfo_S
    {
        double dHz;          /* 主频 */
        int    nTemp;        /* 温度 */
        double dPercent;     /* 利用率 */
        int    nBitWidth;    /* 已用 */
        double dComputility; /* 算力 */

        void clear()
        {
            dHz          = 0.0;
            nTemp        = 0;
            dPercent     = 0.0;
            nBitWidth    = 0.0;
            dComputility = 0.0;
        }

        void print() const
        {
            std::cout << "\n获取GPU信息请求:=============" << std::endl;
            std::cout << "主频:" << dHz << std::endl;
            std::cout << "温度:" << nTemp << std::endl;
            std::cout << "利用率:" << dPercent << std::endl;
            std::cout << "位宽:" << nBitWidth << std::endl;
            std::cout << "算力:" << dComputility << std::endl;
            std::cout << "end:=============" << std::endl;
        }
    };

    struct GrapMemoryInfo_S
    {
        double dBandWidth; /* 带宽 */
        double dPercent;   /* 利用率 */
        double dUsed;      /* 已用 */
        double dTotal;     /* 总共 */

        void clear()
        {
            dBandWidth = 0.0;
            dPercent   = 0.0;
            dUsed      = 0.0;
            dTotal     = 0.0;
        }

        void print() const
        {
            std::cout << "\n获取显存信息请求:=============" << std::endl;
            std::cout << "带宽:" << dBandWidth << std::endl;
            std::cout << "利用率:" << dPercent << std::endl;
            std::cout << "已用:" << dUsed << std::endl;
            std::cout << "总共:" << dTotal << std::endl;
            std::cout << "end:=============" << std::endl;
        }
    };

    struct PerfInfo_S
    {
        CpuInfo_S        stCpuInfo;
        MemoryInfo_S     stMemoryInfo;
        GpuInfo_S        stGpuInfo;
        GrapMemoryInfo_S stGrapMemInfo;

        void clear()
        {
            stCpuInfo.clear();
            stMemoryInfo.clear();
            stGpuInfo.clear();
            stGrapMemInfo.clear();
        }
    };
}    // namespace Ai0630_NS
