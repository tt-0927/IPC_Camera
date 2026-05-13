
#include "ImageAnomaly.h"


int main()
{
	/* 输入图片 */
	cv::Mat aSrcImg = cv::imread("test.jpg");
	using namespace ImageAnomaly_NS;
	// 图像异常检测算法
	cImageAnomaly demo;

	/* 定义背景颜色的范围 */
	cv::Scalar lowerColor(0, 120, 70);
	cv::Scalar upperColor(10, 255, 255);

	/* 去背景 */
	cv::Mat colorMask, anomalyMask, anomalyFrame;
	demo.detectRedAnomaly(aSrcImg, lowerColor, upperColor, colorMask, anomalyMask, anomalyFrame);

	/* 条纹检测 */
	std::vector<cv::Vec4i> lines;
	demo.detectStripes(anomalyFrame, lines);
	/* 绘制检测到的直线 */
	/*for (size_t i = 0; i < lines.size(); i++) 
	{
		cv::Vec4i l = lines[i];
		cv::line(aSrcImg, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), cv::Scalar(255), 2, cv::LINE_AA);
	}
	if(lines.size()>0)
	{
		printf("============== 检测到条纹[%d] =====================\n",lines.size());
	}*/

	/* 亮暗检测算法 */
	/*double AreaBrightness = demo.getLight(aSrcImg, colorMask);
	std::cout<<"当前平均亮度为：=========== "<< AreaBrightness<<" =========" <<std::endl;*/

	/* 噪点检测算法 */
	/*int nZs = demo.countNoise(anomalyMask);
	float fZb = nZs*1.0/anomalyMask.total();
	std::cout<<"噪点检测算法占比以及占比个数：=========== "<< fZb << ";"<< nZs <<" =========" <<std::endl;*/

	/* 图片模糊度检测 */
	double dMh = demo.assessImageSharpness(aSrcImg);
	std::cout<<"图片模糊度程度：=========== "<< dMh <<" =========" <<std::endl;

	return 0;
}
