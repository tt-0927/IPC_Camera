#include "fastpose_zerocopy.h"
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include <vector>

using namespace Fastpose_NS;
int main()
{
	cFastpose demo("./weights/Fastpose.rknn");
	std::vector<float> pOutputs;
	cv::Mat img = cv::imread("test.jpg");
	demo.Infer(img,pOutputs);
	for(int p=0;p<26;p++)
	{
		float x = pOutputs[p*3+0]*(img.cols*1.0/192);
		float y = pOutputs[p*3+1]*(img.rows*1.0/256);
		float conf = pOutputs[p*3+2];
		if(conf >0.8)
		{
			cv::circle(img,cv::Point(x,y),10,(255,0,0),-1);
		}
		printf("[%f,%f,%f]\n",x,y,conf);
	}
	cv::imwrite("OutImage.jpg",img);
	return 0;
}
