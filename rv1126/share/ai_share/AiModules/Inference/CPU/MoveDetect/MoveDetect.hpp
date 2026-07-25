#include <opencv2/opencv.hpp>
#include <iostream>

namespace Inference_NS
{
    class CMoveDetect
    {
    public:
        CMoveDetect();

        ~CMoveDetect();

        bool inference(cv::Mat &frontMat, cv::Mat &afterMat, std::vector<std::vector<int>> &output);

        void setParam(int erode_size, int dilate_size);

    public:
       int erode_size = 1;
       int dilate_size = 1;

    private:
        cv::Mat erode_kernel;
        cv::Mat dilate_kernel;
    };

}
