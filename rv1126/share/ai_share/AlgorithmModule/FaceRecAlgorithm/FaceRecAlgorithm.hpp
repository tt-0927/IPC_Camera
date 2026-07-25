/*
 * @FilePath     : FaceRecAlgorithm.hpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-08 15:11:10
 * @LastEditors  : yanzeh yanzeh@kfb.cn
 * @LastEditTime : 2024-01-18 11:00:53
 * @Description  : 人脸识别模块使用类
 */
#pragma once

#include "FaceRecAlgorithmV1.hpp"
#include "FaceRecExtern.hpp"

namespace FR_NS
{
    class CFaceRecAlgorithm
    {
    public:

        static CFaceRecAlgorithmBase* createFaceRecAlgorithm(FaceRecInParam_S stInfo)
        {
            switch (stInfo.stNeedParam.enType)
            {
                case FR_V1:
                {
                    return new CFaceRecAlgorithmV1(stInfo);
                }
                default:
                {
                    return nullptr;
                }
            }
        }
    };

}    // namespace FR_NS
