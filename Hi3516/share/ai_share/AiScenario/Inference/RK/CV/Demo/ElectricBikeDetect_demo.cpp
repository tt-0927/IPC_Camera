
#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "CVInferenceRK_V1_0.hpp"
#include "ElectricBikeDetect.hpp"

using namespace InferenceV1_0_NS;

cv::Mat resizeAndPadImage(
	const cv::Mat inputImage,
	int imageWidth,
	int imageHeight,
	// int& xOffset,
	// int& yOffset,
	float& resize_scale)
{
	resize_scale = 640.0f / std::max(imageWidth, imageHeight);

	int newWidth = static_cast<int>(imageWidth * resize_scale);
	int newHeight = static_cast<int>(imageHeight * resize_scale);

	cv::Mat resizedImage;
	cv::resize(inputImage, resizedImage, cv::Size(newWidth, newHeight));

	cv::Mat outputImage = cv::Mat::zeros(640, 640, inputImage.type());

	// xOffset = static_cast<int>((640 - newWidth) / 2);
	// yOffset = static_cast<int>((640 - newHeight) / 2);

	resizedImage.copyTo(outputImage(cv::Rect(0, 0, newWidth, newHeight)));

	return outputImage;
}


int main()
{
	CCVInferenceBase* demo = new CElectricBikeDetect("./ElectricBikeDetect.rknn");
	bool bT = demo->init();
	if(!bT)
	{
		printf("初始化参数识别\n");
		exit(0);
	}
	
	AiScenario_NS::CVData_S stInData;
	cv::Mat img = cv::imread("test.png");

	int Width = img.cols;         // 图片的宽
	int Height = img.rows;        // 图片的高
	int xOff = 0;            // 缩放填充后左上角的x坐标
	int yOff = 0;            // 缩放填充后左上角的y坐标
	float scale = 0.0;   // 缩放比例
	cv::Mat imageRGB;           // 图片的RGB格式

	cv::cvtColor(img, imageRGB, cv::COLOR_BGR2RGB);
	stInData.inMat = resizeAndPadImage(imageRGB, Width, Height,
					 				   scale);

    std::vector<float> vOutData;
    bool result = demo->inference(stInData,vOutData);

    for(int i=0;i<vOutData.size()/6;i++)
    {
    	int x1 = static_cast<int>((vOutData[i*6+0]) / scale);
        int y1 = static_cast<int>((vOutData[i*6+1]) / scale);
        int x2 = static_cast<int>((vOutData[i*6+2]) / scale);
        int y2 = static_cast<int>((vOutData[i*6+3]) / scale);

		std::cout << "--: " << x1 << ", " << y1 << ", " << x2 << ", " << y2 << ", " << vOutData[i*6+4] << ", " << vOutData[i*6+5] << ", " << std::endl;
		if (static_cast<int>(vOutData[i*6+5]) == 1)
		{
			cv::rectangle(img, cv::Point(x1,y1), cv::Point(x2,y2), cv::Scalar(0,0,255), 2);
		} else {
			cv::rectangle(img, cv::Point(x1,y1), cv::Point(x2,y2), cv::Scalar(255,0,0), 2);
		}
    	
    }
    cv::imwrite("./ElectricBikeDetect.jpg", img);
    
    delete demo;
    
	return 0;
}
