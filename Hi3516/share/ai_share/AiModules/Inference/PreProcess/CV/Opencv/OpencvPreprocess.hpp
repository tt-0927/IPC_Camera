/**
 * @file OpencvPreprocess.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-04-24
 * 
 * @brief 
 */

 #pragma once

 #include "opencv2/core/core.hpp"
 #include "opencv2/imgcodecs.hpp"
 #include "opencv2/imgproc.hpp"
 #include <opencv2/dnn.hpp>
 #include <vector>
 
 namespace PreProcess_NS
 {
     class COpencvPreProcess
     {
     public:
         /**
          * @brief
          * @param aInput 输入的opencv格式图片
          * @param aOutput 处理后的数据
          * @param vfMean 均值
          * @param vfStd 方差
          * @param bRgb 是否为rgb
          * @return
          */
         bool PreProcess(
            cv::Mat aInput,
            cv::Mat& aOutput,
            std::vector<float> vfMean,
            std::vector<float> vfStd,
            bool bRgb
        );
  
         COpencvPreProcess();
         ~COpencvPreProcess();

     };
 } // namespace PreProcess_NS
 