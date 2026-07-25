/*
 * @FilePath     : BridgeFracture.h
 * @Author       : 吴才朋
 * @Date         : 2024-02-02 09:10:15
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-03-14 10:08:20
 * @Description  : 断桥检测的定义头文件
 */

#ifndef __RK_BRIDGE_FRACTURE_H__
#define __RK_BRIDGE_FRACTURE_H__

#include <dlfcn.h>
#include <fstream>
#include <iostream>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/time.h>
#include <vector>

#include "opencv2/core/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"

namespace BRIDGEFRACTURE_NS
{
    class CBridgeFracture
    {
    private:

        /* 记录 标准线的各个点累计数量 */
        std::vector<std::map<int, int>> m_vLinesCounts;
        /* 记录 标准线的两点坐标 */
        std::vector<std::vector<int>>   m_vLinePoints;
        /* 断桥的位置 */
        std::vector<std::vector<int>>   m_vBridgeFractureAreas;
        /* 分析直线 */
        std::vector<int>   m_vBridgeLines;

        /* 断桥的数量 */
        int m_nBridgeFractureNum = 0;

        /* ============ 可调参数 ================ */
        /* 其他线段和 标准线 的距离阈值 */
        int m_nDistanceThreshold = 10;

        /* 其他线段和 标准线 的角度阈值 */
        int m_nAngleThreshold = 5;

        /* 累计断桥数的上限 */
        int m_nBrokenBridgeNumThreshold = 20;

        /* 概率霍夫直线的相关参数 */
        double m_dThreshold     = 20;
        double m_dMinLineLength = 10;
        double m_dMaxLineGap    = 10;


        /* 筛选出跟 基准线同一个方向且在附近的 线 */
        void filterLines(cv::Mat aHfLines, std::vector<int> vLinePoints, std::map<int, int>& mLinesCount, cv::Mat& aFrame);

    public:

        /**
         * @brief 对输入的线段两点，生成线段内一系列的连续的x坐标
         * @param [std::vector<int>] vnInputLinePoints: 包含两点的坐标
         * @note
         */
        bool setData(std::vector<int> vnInputLinePoints);
        bool setData(std::vector<std::vector<int>> vvnInputLinePoints);

        /**
         * @brief 清除基准线
         * @return [*]
         * @note
         */
        void clearData();

        /**
         * @brief 通过算法判断断桥的位置
         * @param [cv::Mat] aFrame: opencv数据
         * @note
         */
        bool detectBridgeFracture(cv::Mat& aFrame);

        /**
         * @brief 获取断桥的位置
         * @param [std::vector<std::vector<int>>] vBridgeFractureAreas: 断桥的位置容器
         * @note
         */
        void getBridgeFractureAreas(std::vector<std::vector<int>>& vBridgeFractureAreas)
        {
            vBridgeFractureAreas = m_vBridgeFractureAreas;
        }

        /**
         * @brief 获取分析的直线
         * @param [std::vector<int>] vBridgeLines: 直线
         * @note
         */
        void getBridgeLines(std::vector<int>& vBridgeLines)
        {
            vBridgeLines = m_vBridgeLines;
        }

        /* 构造函数 */
        CBridgeFracture();
        CBridgeFracture(int nDistanceThreshold, int nAngleThreshold, int nBrokenBridgeNumThreshold);
        /* 析构函数 */
        ~CBridgeFracture();
    };
}    // namespace BRIDGEFRACTURE_NS

#endif    // __RK_BRIDGE_FRACTURE_H__
