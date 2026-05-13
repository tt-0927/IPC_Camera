/*
 * @FilePath     : ColorCutout.hpp
 * @Author       : 吴才朋 wucp@kfb.cn
 * @Date         : 2024-08-20 08:40:01
 * @LastEditors: 李辉 lihui@kfb.cn
 * @LastEditTime: 2024-10-31 16:27:08
 * @Description  : 人像抠图
 */
#pragma once

#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "dlog.h"
#include "CVExtern.hpp"

namespace ColorCutout_NS
{ 
	class CColorCutout
	{
    public:
        CColorCutout();
        ~CColorCutout();

    private:
		/* 人像抠图公式：(R+G-2B+fThreshold1)*(1+fThreshold2) */
		cv::Mat cutoutFormula(cv::Mat aImage, AiScenario_NS::CutoutParam_S sCutoutParam);
    public:
		/* 抠图推理 */
		int inference(AiScenario_NS::CutoutParam_S& sCutoutParam);
	};
}

