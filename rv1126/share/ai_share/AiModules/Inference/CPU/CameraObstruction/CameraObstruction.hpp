#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <math.h>
#include <iostream>

namespace Inference_NS
{
    class CComplexityDetect
    {
    public:
        CComplexityDetect();
        ~CComplexityDetect();

    public:
        cv::Size aResizeSet = cv::Size{160, 120}; /* 输入推理的大小 */
        cv::Size aSuperpixelSet = cv::Size{12, 9};  /* 划分成多少个格子检测 */

    public:
        /**
         * @brief 摄像头画面模糊检测（遮挡会模糊）
         * @param aImage 摄像头画面
         * @param dResult 返回模糊的成都
         * @return true 
         * @return false 
         */
        bool inference(cv::Mat aImage, double &dResult);
    };
}
